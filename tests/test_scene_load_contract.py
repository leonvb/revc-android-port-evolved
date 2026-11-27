"""Guard the scripted scene-load timer pairing against regressions."""
from pathlib import Path

source = Path('src/control/Script4.cpp').read_text()
case = source.split('case COMMAND_LOAD_SCENE:', 1)[1].split('case COMMAND_ADD_STUCK_CAR_CHECK:', 1)[0]
load = case.index('CStreaming::LoadScene(pos);')
assert case[:load].count('CTimer::Suspend();') == 1
assert case[load:].count('CTimer::Resume();') == 1
assert 'CTimer::Suspend();' not in case[load:]
