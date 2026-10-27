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
  mutate:s=>s.replace('return 2 + m_iEscortCount;','return 1 + m_iEscortCount;')},
 {name:'unknown operation types admitted',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationManagerComponent.c',suite:'Test-MarsAndAttacks.js',
  mutate:s=>s.replace('if (type != AROR_EOperationType.ATTACK && type != AROR_EOperationType.QRF)','if (false)')},
 {name:'withdrawal dispatches ready reserves',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationExecutorComponent.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('if (tactical && tactical.m_bWithdraw)','if (false)')},
 {name:'foreign driver accepted as ready',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_GroundOperationForce.c',suite:'Test-QRFBoarding.js',
  mutate:s=>s.replace('return ownDriver;','return true;')},
 {name:'unissued infantry group skipped by retention',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationExecutorComponent.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('if (force.m_bOrdersIssued || force.m_Group)','if (force.m_bOrdersIssued)')},
 {name:'aborted materialized aircraft deleted immediately',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_ReaperAirForce.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('if (materialized && !m_bImmediateCleanup)','if (m_bOrdersIssued && !m_bImmediateCleanup)')},
 {name:'queued ground origin is not rechecked',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationExecutorComponent.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('if (!TacticalOriginAvailable(operation))','if (false)')},
 {name:'air construction ignores changed player clearance',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_ReaperQRFExecutor.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('if (!IsLaunchPositionSafe(force.m_vLaunch, force.m_vTown) || !AROR_ReaperFlightGeometry.IsAirspaceClear(force.m_vLaunch, m_fReaperInsertionHeight + force.m_iPackageIndex * 15, 14))','if (false)')},
 {name:'funding shortage becomes configuration failure',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_ReaperQRFExecutor.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('profile = fallback;','profile = null;')},
 {name:'player aircraft occupancy ignored',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_ReaperAirForce.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace(/(static bool HasPlayerOccupant[\s\S]*?)return true;/,'$1return false;')},
 {name:'activation overhead charged twice',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_QRFFinance.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace(' || account.m_bActivated','')},
 {name:'closed account keeps unbuilt collateral',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_QRFFunding.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('if (!m_bClosed)','if (true)')},
 {name:'fixed-task exits retry without a bound',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationExecutorComponent.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace(' && force.m_iTacticalExitRetries < m_iQRFExitRetryLimit','')},
 {name:'nested finance traversal removes an account twice',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_QRFFinance.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace(' || m_bReconciling','')},
 {name:'reserve window expires before gunship egress',file:'AROpenResistance/Open Resistance/Worlds/AROR_Everon/AROR_Everon_Chicken_Layers/Managers.layer',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('m_iQRFReserveWait 450','m_iQRFReserveWait 300')},
 {name:'partially validated aircraft have no boarding hold',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_ReaperQRFExecutor.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('force.m_bAircraftHeld = true;','force.m_bAircraftHeld = false;')},
 {name:'empty failed construction keeps its collateral',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationExecutorComponent.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('if (emptyOwnedMembers.IsEmpty())','if (false)')},
 {name:'unused partial-package collateral never released',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_QRFFunding.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('m_iQuote = Math.Max(0, m_iQuote - Math.Max(0, packageQuote - registered));','m_iQuote = m_iQuote;')},
 {name:'virtual proxy retains package finalization ownership',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationExecutorComponent.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('reservation.m_bOwnsPackageQuote = false;','reservation.m_bOwnsPackageQuote = true;')},
 {name:'escort child finalizes transport collateral',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_ReaperQRFExecutor.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('force.m_Escort.m_bOwnsPackageQuote = false;','force.m_Escort.m_bOwnsPackageQuote = true;')},
 {name:'player-owned pending spawn queue releases collateral early',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_OperationForce.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace(' && !m_bPackageQueuesPreserved','')},
 {name:'escort player queue not propagated to package owner',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_ReaperAirForce.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('if (m_Escort.m_bPackageQueuesPreserved)','if (false)')},
 {name:'retirement orders unissued escort to fly',file:'AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_ReaperRetirement.c',suite:'Test-QRFInterruptions.js',
  mutate:s=>s.replace('if (force.m_bOrdersIssued && !force.m_bEscortLost','if (!force.m_bEscortLost')}
];
function runQRFMutationTests(read,execute){
 const results=[];
 for(const test of cases){
  const original=read(test.file),mutated=test.mutate(original);
  if(original===mutated)throw Error('Mutation did not match current source: '+test.name);
  const result=execute(test.suite,{[test.file]:mutated});
  if(result.passed)throw Error('Regression survived: '+test.name);
  const lines=String(result.error).split('\n');
  results.push({name:test.name,suite:test.suite,killed:true,error:lines.find(line=>/^(?:\w*Error):/.test(line))||lines[0]});
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
