import importlib.util
from pathlib import Path
ROOT=Path(__file__).resolve().parent
spec=importlib.util.spec_from_file_location('workshop_server',ROOT/'server.py')
module=importlib.util.module_from_spec(spec)
spec.loader.exec_module(module)
base={'rate':240,'window':3,'simrate':240,'duration':1.1,'targets':[240],
      'events':[{'time':237/240,'down':True},{'time':238/240,'down':False}]}
result=module.simulate(base)
assert result['metrics']['corrected']==1
assert len(result['events'])==2
assert abs(result['events'][0]['time']-1)<1e-9
assert abs(result['events'][1]['time']-241/240)<1e-9
bypass=module.simulate({**base,'enabled':False})
assert bypass['metrics']['corrected']==0
for actual,physical in zip(bypass['events'],base['events']):
    assert actual['down']==physical['down']
    assert abs(actual['time']-physical['time'])<1e-9
for invalid in [[],{'duration':float('nan')},{'targets':[-1]},{'events':[{'time':-1,'down':True}]}]:
    try:module.simulate(invalid)
    except (ValueError,TypeError,KeyError):pass
    else:raise AssertionError('invalid request accepted')
print('native bridge checks passed | engine '+result['engineHash'])
