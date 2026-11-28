package com.revc.game;

import android.Manifest;
import android.app.AlertDialog;
import android.content.Intent;
import android.content.pm.PackageManager;
import android.content.res.AssetManager;
import android.net.Uri;
import android.os.Build;
import android.os.Bundle;
import android.os.Environment;
import android.provider.Settings;
import android.widget.Toast;

import android.view.Display;
import android.view.Surface;
import android.view.ViewGroup;
import android.view.WindowManager;

import androidx.core.app.ActivityCompat;
import androidx.core.content.ContextCompat;

import org.libsdl.app.SDLActivity;

import java.io.File;
import java.io.FileOutputStream;
import java.io.IOException;
import java.io.InputStream;
import java.io.OutputStream;

/**
 * Actividad principal del juego.
 *
 * Le indica al motor nativo (via el argumento "--dir", que ya soporta en
 * todas las plataformas) que busque los archivos del juego (data/, models/,
 * audio/, anim/, etc.) en una carpeta simple y visible en cualquier
 * explorador de archivos: el almacenamiento interno del dispositivo,
 * carpeta "reVC" (por ej. "Almacenamiento interno/reVC").
 *
 * Evita depender de Android/data/com.revc.game/files o de un OBB, que la
 * mayoría de usuarios no sabe ubicar ni gestionar.
 */
public class GameActivity extends SDLActivity {

    /**
     * Nombre de la carpeta con los archivos del juego, en la raíz del almacenamiento
     * interno. Viene de BuildConfig (ver productFlavors en build.gradle) para que
     * cada edición (standard, longnight, ...) use su propia carpeta sin tocar la
     * de las demás -- son apps instaladas por separado (applicationId distinto).
     */
    public static final String GAME_FOLDER_NAME = BuildConfig.GAME_FOLDER_NAME;

    private static final int REQUEST_LEGACY_STORAGE_PERMISSION = 1001;
    private boolean missingGameFiles = false;
    private volatile int requestedRefreshRateHz = 30;

    public static File getGameDir() {
        return new File(Environment.getExternalStorageDirectory(), GAME_FOLDER_NAME);
    }

    @Override
    protected void onCreate(Bundle savedInstanceState) {
        // Android exige llamar a super.onCreate() siempre, sin importar qué
        // rama tomemos después (si no, ActivityThread tira
        // SuperNotCalledException y la app crashea antes de mostrar nada).
        // SDLActivity.onCreate() carga las libs nativas y prepara la
        // superficie, pero el hilo nativo (SDLThread, donde arranca
        // realmente el motor) no se lanza ahí -- se lanza recién en
        // handleNativeState() cuando la superficie está lista Y onResume()
        // ya se llamó. Por eso, si falta el permiso, alcanza con hacer
        // finish() antes de que la activity llegue a onResume(): la
        // superficie nunca se vuelve válida y el motor nunca llega a
        // intentar leer archivos que no puede.
        super.onCreate(savedInstanceState);

        File gameDir = getGameDir();
        if (!gameDir.exists()) {
            gameDir.mkdirs();
        }

        if (!hasFullStorageAccess()) {
            requestFullStorageAccess();
            // El usuario reabre la app luego de conceder el permiso.
            finish();
            return;
        }

        ensureGameControllerDb(gameDir);
        ensureBundledRevcFiles(gameDir);

        if (!hasPcGameData(gameDir)) {
            missingGameFiles = true;
            new AlertDialog.Builder(this)
                    .setTitle("GTA: Vice City files not found")
                    .setMessage("Copy your PC Grand Theft Auto: Vice City game files (models/, data/, audio/, anim/, txd/, etc.) into:\n\n"
                            + gameDir.getAbsolutePath()
                            + "\n\n(Internal storage / " + GAME_FOLDER_NAME + ")\n\nand reopen reVC.")
                    .setCancelable(false)
                    .setPositiveButton("Exit", (d, which) -> finish())
                    .show();
            return;
        }

        // Draw the game (and our touch controls) behind the camera cutout
        // consistently in landscape, instead of the system letterboxing
        // around it -- our TouchControlsView reads the actual safe-area
        // insets and steers controls clear of it either way, but the game
        // view itself should still fill the whole screen.
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.P) {
            getWindow().getAttributes().layoutInDisplayCutoutMode =
                    WindowManager.LayoutParams.LAYOUT_IN_DISPLAY_CUTOUT_MODE_SHORT_EDGES;
        }

        // libreVC.so is already loaded at this point (loadLibraries(), called
        // from within super.onCreate(), just did it), so TouchControlsView's
        // native methods resolve fine without a separate System.loadLibrary.
        mLayout.addView(new TouchControlsView(this),
                new ViewGroup.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, ViewGroup.LayoutParams.MATCH_PARENT));
    }

    @Override
    protected void resumeNativeThread() {
        if (missingGameFiles) {
            return;
        }
        applyDisplayRefreshRate(requestedRefreshRateHz);
        super.resumeNativeThread();
    }

    @Override
    public void onWindowFocusChanged(boolean hasFocus) {
        if (missingGameFiles) {
            return;
        }
        if (hasFocus) {
            applyDisplayRefreshRate(requestedRefreshRateHz);
        }
        super.onWindowFocusChanged(hasFocus);
    }

    /**
     * Called from C++ (CJavaWrapper::SetTargetRefreshRate) on the SDL thread
     * whenever the user changes Frame rate in reVC settings.
     * fps <= 0 hints a high display refresh; gameplay has no software cap.
     * Android and the display may choose a different mode.
     */
    public void setTargetRefreshRate(final int fps) {
        requestedRefreshRateHz = fps;
        runOnUiThread(() -> applyDisplayRefreshRate(requestedRefreshRateHz));
    }

    private void applyDisplayRefreshRate(int targetFps) {
        if (isFinishing() || isDestroyed()) {
            return;
        }

        Display display = (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R)
                ? getDisplay()
                : getWindowManager().getDefaultDisplay();

        float chosenHz = (targetFps > 0) ? (float) targetFps : 0.0f;
        int bestModeId = 0;

        if (display != null && Build.VERSION.SDK_INT >= Build.VERSION_CODES.M) {
            Display.Mode currentMode = display.getMode();
            Display.Mode[] modes = display.getSupportedModes();
            if (currentMode != null && modes != null && modes.length > 0) {
                int curW = currentMode.getPhysicalWidth();
                int curH = currentMode.getPhysicalHeight();

                if (targetFps <= 0) {
                    // Native mode: select highest refresh rate matching current physical resolution.
                    float maxHz = 0.0f;
                    for (Display.Mode m : modes) {
                        if (m.getPhysicalWidth() == curW && m.getPhysicalHeight() == curH) {
                            if (m.getRefreshRate() > maxHz) {
                                maxHz = m.getRefreshRate();
                                bestModeId = m.getModeId();
                            }
                        }
                    }
                    chosenHz = maxHz;
                } else {
                    // Prefer an exact or integer-multiple refresh mode (e.g. 60 Hz or 120 Hz for 60 FPS,
                    // 120 Hz or 60 Hz for 30 FPS) so frame pacing is judder-free on LTPO panels.
                    float bestScore = Float.MAX_VALUE;
                    for (Display.Mode m : modes) {
                        if (m.getPhysicalWidth() != curW || m.getPhysicalHeight() != curH) {
                            continue;
                        }
                        float hz = m.getRefreshRate();
                        float directDiff = Math.abs(hz - targetFps);
                        float score;
                        if (directDiff < 1.5f) {
                            // Exact panel refresh match (e.g. 60 Hz for 60 FPS, 120 Hz for 120 FPS)
                            score = directDiff;
                        } else if (hz >= targetFps) {
                            int mult = Math.round(hz / (float) targetFps);
                            if (mult >= 1 && Math.abs(hz - (targetFps * mult)) < 2.0f) {
                                // Clean integer multiple (e.g. 60 Hz for 30 FPS -> mult=2)
                                score = 10.0f + mult;
                            } else {
                                score = 100.0f + (hz - targetFps);
                            }
                        } else {
                            // Lower than requested FPS: only pick if nothing >= targetFps exists
                            score = 1000.0f + (targetFps - hz);
                        }

                        if (score < bestScore) {
                            bestScore = score;
                            bestModeId = m.getModeId();
                            chosenHz = hz;
                        }
                    }
                }
            }
        }

        WindowManager.LayoutParams lp = getWindow().getAttributes();
        boolean changed = false;
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.M && bestModeId != 0 && lp.preferredDisplayModeId != bestModeId) {
            lp.preferredDisplayModeId = bestModeId;
            changed = true;
        }
        if (chosenHz > 0.0f && Math.abs(lp.preferredRefreshRate - chosenHz) > 0.1f) {
            lp.preferredRefreshRate = chosenHz;
            changed = true;
        }
        if (changed) {
            getWindow().setAttributes(lp);
        }

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R && mSurface != null) {
            Surface surface = mSurface.getNativeSurface();
            if (surface != null && surface.isValid()) {
                float surfaceFps = (targetFps > 0) ? (float) targetFps : 0.0f;
                int compatibility = (targetFps > 0)
                        ? Surface.FRAME_RATE_COMPATIBILITY_FIXED_SOURCE
                        : Surface.FRAME_RATE_COMPATIBILITY_DEFAULT;
                try {
                    surface.setFrameRate(surfaceFps, compatibility);
                } catch (Throwable ignored) {
                }
            }
        }
    }

    @Override
    protected String[] getArguments() {
        return new String[]{"--dir", getGameDir().getAbsolutePath()};
    }

    @Override
    protected String[] getLibraries() {
        // El target de CMake se llama "reVC" (mayúscula incluida, ver
        // /CMakeLists.txt), por lo que el binario resultante es
        // "libreVC.so". El nombre acá debe coincidir EXACTO: Android es
        // case-sensitive y System.loadLibrary("revc") jamás encontraría
        // "libreVC.so".
        return new String[]{"SDL2", "openal", "mpg123", "reVC"};
    }

    /**
     * gamecontrollerdb.txt es la base de mapeos de SDL2 para reconocer
     * mandos físicos (sobre todo Bluetooth genéricos que Android no
     * identifica como "GameController" de por sí -- sin esta base, SDL2 no
     * sabe qué botón físico corresponde a cada botón lógico, D-Pad
     * incluido). No es contenido del juego -- es la base de datos
     * open-source de SDL (https://github.com/mdqinc/SDL_GameControllerDB,
     * licencia MIT), así que la empaquetamos en la app y la copiamos sola
     * a la carpeta del juego si todavía no está, en vez de depender de que
     * el usuario la traiga junto con sus propios archivos.
     */
    private void ensureGameControllerDb(File gameDir) {
        File dst = new File(gameDir, "gamecontrollerdb.txt");
        if (dst.exists()) return;
        try (InputStream in = getAssets().open("gamecontrollerdb.txt");
             OutputStream out = new FileOutputStream(dst)) {
            byte[] buf = new byte[8192];
            int n;
            while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
        } catch (IOException e) {
            // No crítico: sin esto, algunos mandos genéricos no se
            // reconocen, pero el juego sigue funcionando con los que sí
            // (y con los controles táctiles) igual.
        }
    }

    /**
     * Extract bundled reVC-specific runtime files (gamefiles/ -> neo/, TEXT/,
     * models/frontend_*.txd, etc.) into gameDir when missing, without
     * overwriting files the user placed themselves.
     */
    private void ensureBundledRevcFiles(File gameDir) {
        copyAssetTree(getAssets(), "gamefiles", gameDir);
    }

    private void copyAssetTree(AssetManager assets, String assetPath, File dstDir) {
        String[] entries;
        try {
            entries = assets.list(assetPath);
        } catch (IOException e) {
            return;
        }
        if (entries == null || entries.length == 0) {
            File existing = findCaseInsensitiveChild(dstDir.getParentFile(), dstDir.getName());
            if (existing != null && existing.exists()) {
                return;
            }
            File parent = dstDir.getParentFile();
            if (parent != null && !parent.exists()) {
                parent.mkdirs();
            }
            try (InputStream in = assets.open(assetPath);
                 OutputStream out = new FileOutputStream(dstDir)) {
                byte[] buf = new byte[8192];
                int n;
                while ((n = in.read(buf)) > 0) out.write(buf, 0, n);
            } catch (IOException ignored) {
            }
            return;
        }

        File targetDir = dstDir;
        if (dstDir.getParentFile() != null && !"gamefiles".equals(assetPath)) {
            File ciDir = findCaseInsensitiveChild(dstDir.getParentFile(), dstDir.getName());
            if (ciDir != null && ciDir.isDirectory()) {
                targetDir = ciDir;
            }
        }
        if (!targetDir.exists()) {
            targetDir.mkdirs();
        }
        for (String name : entries) {
            copyAssetTree(assets, assetPath + "/" + name, new File(targetDir, name));
        }
    }

    private File findCaseInsensitiveChild(File dir, String name) {
        if (dir == null || !dir.isDirectory()) return null;
        File exact = new File(dir, name);
        if (exact.exists()) return exact;
        File[] list = dir.listFiles();
        if (list == null) return null;
        for (File f : list) {
            if (f.getName().equalsIgnoreCase(name)) {
                return f;
            }
        }
        return null;
    }

    private File resolvePathCaseInsensitive(File root, String... parts) {
        File cur = root;
        for (String part : parts) {
            cur = findCaseInsensitiveChild(cur, part);
            if (cur == null) return null;
        }
        return cur;
    }

    private boolean hasPcGameData(File gameDir) {
        File gta3Img = resolvePathCaseInsensitive(gameDir, "models", "gta3.img");
        File mainScm = resolvePathCaseInsensitive(gameDir, "data", "main.scm");
        return gta3Img != null && gta3Img.isFile() && mainScm != null && mainScm.isFile();
    }

    private boolean hasFullStorageAccess() {
        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            return Environment.isExternalStorageManager();
        }
        return ContextCompat.checkSelfPermission(this, Manifest.permission.WRITE_EXTERNAL_STORAGE)
                == PackageManager.PERMISSION_GRANTED;
    }

    private void requestFullStorageAccess() {
        Toast.makeText(this,
                "Concedé el permiso \"Todos los archivos\" y volvé a abrir reVC.\n"
                        + "Copiá los archivos del juego en: Almacenamiento interno/" + GAME_FOLDER_NAME,
                Toast.LENGTH_LONG).show();

        if (Build.VERSION.SDK_INT >= Build.VERSION_CODES.R) {
            try {
                Intent intent = new Intent(Settings.ACTION_MANAGE_APP_ALL_FILES_ACCESS_PERMISSION,
                        Uri.parse("package:" + getPackageName()));
                startActivity(intent);
            } catch (Exception e) {
                startActivity(new Intent(Settings.ACTION_MANAGE_ALL_FILES_ACCESS_PERMISSION));
            }
        } else {
            ActivityCompat.requestPermissions(this,
                    new String[]{Manifest.permission.READ_EXTERNAL_STORAGE, Manifest.permission.WRITE_EXTERNAL_STORAGE},
                    REQUEST_LEGACY_STORAGE_PERMISSION);
        }

        finish();
    }
}
