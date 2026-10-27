// Mutation checks prove selected regressions are caught; source files are never edited.
const cases=[
 {name:'asynchronous pilot requests treated as paused=false',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_ReaperAirForce.c',suite:'Test-QRFReleaseLifecycle.js',
  mutate:s=>s.replace('access.GetInVehicle(vehicle, slot, true, -1, ECloseDoorAfterActions.CLOSE_DOOR, true);','access.GetInVehicle(vehicle, slot, true, -1, ECloseDoorAfterActions.CLOSE_DOOR, false);')},
 {name:'unready reserve dispatched',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationExecutorComponent.c',suite:'Test-QRFReleaseLifecycle.js',
  mutate:s=>s.replace(' || !ready.m_bReady','')},
 {name:'ground staging bypassed',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationExecutorComponent.c',suite:'Test-QRFReleaseLifecycle.js',
  mutate:s=>s.replace('if (force.m_bTacticalStaging)','if (false)')},
 {name:'town pacification disconnected',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationExecutorComponent.c',suite:'Test-QRFReleaseLifecycle.js',
  mutate:s=>s.replace('if (target && commander.m_iTownHoldStarted >= 0 && !commander.m_bWithdraw)','if (false)')},
 {name:'capture activation recorded after subscribers',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_TerritoryAttackManager.c',suite:'Test-QRFReleaseLifecycle.js',
  mutate:s=>s.replace(/RecordTerritoryOperationChange\(operation\);\r?\n\t\tsuper.NotifyChanged\(operation\);/,'super.NotifyChanged(operation);\n\t\tRecordTerritoryOperationChange(operation);')},
 {name:'debug damage escalates campaign',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationDamageObserver.c',suite:'Test-QRFReleaseLifecycle.js',
  mutate:s=>s.replace('attacker.GetOrigin(), !operation.m_bDebugRequest','attacker.GetOrigin(), true')},
 {name:'admin mutation loses role gate',file:'AROpenResistance/Open Resistance/Scripts/Game/Admin/AROR_AdminMenuComponent.c',suite:'Test-QRFReleaseLifecycle.js',
  mutate:s=>s.replaceAll(' || !AROR_AdminAccess.IsAdministrator(playerId)','')},
 {name:'unbuilt collateral refunded twice',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationExecutorComponent.c',suite:'Test-QRFReleaseLifecycle.js',
  mutate:s=>s.replace(' && pending.m_iReservedMembers > 0','')},
 {name:'air group budget undercounted',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_ForceProfile.c',suite:'Test-MarsAndAttacks.js',
  mutate:s=>s.replace(/if \(m_eType == AROR_EForceType.AIRBORNE\)\r?\n\t\t\treturn 3;/,'if (m_eType == AROR_EForceType.AIRBORNE)\n\t\t\treturn 2;')},
 {name:'unknown operation types admitted',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationManagerComponent.c',suite:'Test-MarsAndAttacks.js',
  mutate:s=>s.replace('if (type != AROR_EOperationType.ATTACK && type != AROR_EOperationType.QRF)','if (false)')}
];
function runQRFMutationTests(read,execute){
 const results=[];
 for(const test of cases){
  const original=read(test.file),mutated=test.mutate(original);
  if(original===mutated)throw Error('Mutation did not match current source: '+test.name);
  const result=execute(test.suite,{[test.file]:mutated});
  if(result.passed)throw Error('Regression survived: '+test.name);
  results.push({name:test.name,suite:test.suite,killed:true,error:String(result.error).split('\n')[0]});
 }
 return {assertions:results.length,results,note:'Intentional mutations in isolated source snapshots; no native behavior or source-file edits.'};
}
if(typeof module!=='undefined')module.exports={runQRFMutationTests};
if(typeof require==='function'&&require.main===module){
 const fs=require('fs'),path=require('path'),vm=require('vm'),root=path.resolve(__dirname,'..');
 const read=p=>fs.readFileSync(path.join(root,p),'utf8');
 const execute=(suite,overrides)=>{
  const mapped=new Map(Object.entries(overrides).map(([p,s])=>[path.resolve(root,p),s]));
  const module={exports:{}},requireInTest=name=>{
   if(name==='fs')return {...fs,readFileSync:(p,...args)=>mapped.get(path.resolve(p))??fs.readFileSync(p,...args)};
   return require(name);
  };
  requireInTest.main=module;
  try{vm.runInNewContext(read('tools/'+suite),{require:requireInTest,module,__dirname,console:{log(){}}},{filename:suite,timeout:30000});return {passed:true}}
  catch(error){return {passed:false,error:error.stack}}
 };
 console.log(JSON.stringify(runQRFMutationTests(read,execute)));
}
