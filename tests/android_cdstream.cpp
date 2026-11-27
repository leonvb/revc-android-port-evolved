// Exercise the actual Android streaming implementation on a POSIX host.
// Link with function-section garbage collection to omit game-only entry points.
#include "../src/core/CdStream_posix.cpp"
#include <cassert>
#include <cstring>
#ifdef main
#undef main
#endif

void re3_debug(const char *, ...) {}
void re3_assert(const char *, const char *, unsigned int, const char *) { abort(); }

int main()
{
	char path[] = "/tmp/revc-cdstream-test-XXXXXX";
	int fd = mkstemp(path);
	assert(fd >= 0);
	unlink(path);
	unsigned char sector[CDSTREAM_SECTOR_SIZE];
	for(int i = 0; i < 32; ++i){
		memset(sector, i, sizeof(sector));
		assert(write(fd, sector, sizeof(sector)) == (ssize_t)sizeof(sector));
	}
	gImgFiles[0] = fd + 1;
	for(int cycle = 0; cycle < 3; ++cycle){
		gNumChannels = 2;
		gpReadInfo = (CdReadInfo *)calloc(gNumChannels, sizeof(CdReadInfo));
		assert(gpReadInfo);
		CdStreamInitThread();
		unsigned char buffers[2][CDSTREAM_SECTOR_SIZE];
		for(int i = 0; i < 3000; ++i){
			int a = i % 32, b = (i * 7 + 1) % 32;
			assert(CdStreamRead(0, buffers[0], a, 1) == STREAM_SUCCESS);
			assert(CdStreamRead(1, buffers[1], b, 1) == STREAM_SUCCESS);
			assert(CdStreamSync(0) == STREAM_NONE);
			assert(CdStreamSync(1) == STREAM_NONE);
			for(size_t j = 0; j < sizeof(sector); ++j){
				assert(buffers[0][j] == a);
				assert(buffers[1][j] == b);
			}
		}
		// A short read must report an error, not pass a partial buffer as data.
		assert(CdStreamRead(0, buffers[0], 32, 1) == STREAM_SUCCESS);
		assert(CdStreamSync(0) == STREAM_ERROR);
		CdStreamShutdown();
	}
	close(fd);
}
