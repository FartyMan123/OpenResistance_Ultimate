/* Actual Enforce method bodies with mocked native services. No engine compile/gameplay claim.
 * Run: node tools/Test-GarrisonsAndSupport.js
 */
function runGarrisonsAndSupportTests(sources) {
  Object.defineProperty(String.prototype,'IsEmpty',{value:function(){return this.length===0;},configurable:true});
  Object.defineProperty(Array.prototype,'IsEmpty',{value:function(){return this.length===0;},configurable:true});
  let assertions=0;
  const check=(value,message)=>{assertions++;if(!value)throw Error(message);};
  let server=true;
  const types={TOWN:0,HQ:1,AIRPORT:2,CITY:3,FACTORY:4,MILITARY_BASE:5,PORT:6,RESOURCES:7,OUTPOST:8};
  const roles={RESISTANCE:0,OCCUPIERS:1,INVADERS:2};
  const env={Replication:{IsServer:()=>server},AROR_ELocationType:types,AROR_EFactionRole:roles,
    Math:{Clamp:(n,a,b)=>Math.max(a,Math.min(b,n)),Max:Math.max,Min:Math.min,AbsFloat:Math.abs,RandomInt:(a,b)=>a,Mod:(a,b)=>a%b},
    vector:{DistanceXZ:(a,b)=>Math.hypot(a[0]-b[0],a[2]-b[2])},
    Print:()=>{},LogLevel:{NORMAL:0,WARNING:1},string:{Empty:'',Format:(text,...args)=>text.replace(/%(\d+)/g,(_,i)=>String(args[i-1]))},
    AROR_GarrisonSlot:class{constructor(){Object.assign(this,{m_bLost:false,m_fHealth:1,m_bSpawned:false,m_Entity:null,m_iGroup:-1,m_iVehicle:-1,m_iSeat:-1,m_bPlacementUnavailable:false,m_iPlacementFailureSince:-1});}},
    AROR_GarrisonPolicy:{},AROR_GarrisonManagerComponent:{GetInstance:()=>({CanDefendRole:()=>true})}};
  const methods=['SetValues','UpdateControl_S','IsEligible','AddSlot','ReservedPersonnel','ReportFailure','AssignMembers','CrewAndOrder','IsPlayerOrForeignAsset','Observe_S','ClearAssignments_S','HasDormantReserve','IsDefencePending','GetDefenceCaptureBlockReason','GetLocation','GetOwner','PlayerNear','FactionFor','HasEstablishedDefence_S'];
  function compile(source,name){
    const m=source.match(new RegExp('^\\t[^\\r\\n]*\\b'+name+'\\(([^\\r\\n]*)\\)\\r?\\n\\t\\{([\\s\\S]*?)^\\t\\}', 'm'));
    if(!m)throw Error('Missing '+name);
    let args=m[1].replace(/\bnotnull /g,'').replace(/array<[^>]+>\s+/g,'').replace(/\b(?:vector|bool|float|int|string|Faction|ResourceName|IEntity|GenericComponent|SaveContext|LoadContext|AROR_\w+)\s+/g,'');
    let body=m[2].replace(/\bfloat\.MAX/g,'Number.MAX_VALUE')
	  .replace(/const AROR_\w+ /g,'let ')
	  .replace(/array<ref AROR_GarrisonSlot> (\w+) = \{\};/g,'let $1=[];')
	  .replace(/array<(?:int|AROR_\w+)> (\w+) = \{\};/g,'let $1=[];')
	  .replace('m_iElapsedMilliseconds / 1000','Math.floor(m_iElapsedMilliseconds / 1000)')
	  .replace(/\b(?:int|string) (\w+(?:, \w+)+);/g,'let $1;')
	  .replace(/context.ReadValue\(([^,]+), ([^)]+)\)/g,'context.ReadValue($1,v=>$2=v)')
	  .replace(/\bstring (\w+);/g,"let $1='';")
      .replace(/foreach \((?:int (\w+), )?\w+ (\w+) : ([^)]+)\)/g,(_,index,value,list)=>index?`for(const [${index},${value}] of ${list}.entries())`:`for(const ${value} of ${list})`)
      .replace(/\b(?:bool|float|int|string|IEntity|DamageManagerComponent|ChimeraCharacter|AROR_\w+) (\w+);/g,'let $1=0;')
      .replace(/\b(?:bool|float|int|string|IEntity|DamageManagerComponent|ChimeraCharacter|AROR_\w+) (\w+) =/g,'let $1 =')
      .replace(/(?<![.\w])m_\w+/g,x=>'this.'+x)
      .replace(/\.Count\(\)/g,'.length')
      .replace(/\.Insert\(/g,'.push(').replace(/\.Compare\(/g,'.localeCompare(');
    for(const method of methods)body=body.replace(new RegExp('(?<![.\\w])'+method+'\\(', 'g'),'this.'+method+'(');
    return new Function(...Object.keys(env),'return function('+args+'){'+body+'}')( ...Object.values(env));
  }
  // Use actual manager methods to exercise arrival inside the site, rather than
  // only an ordinary approach through the activation boundary.
  let playerPosition=[1001,0,0],playerDead=false,ready=true;
  const player={GetOrigin:()=>playerPosition,GetCharacterController:()=>({IsDead:()=>playerDead})};
  const centre={GetCenter:()=>[0,0,0]};
  const playerManager={GetPlayers:out=>out.push(1),GetPlayerControlledEntity:()=>player};
  const game={GetPlayerManager:()=>playerManager};
  Object.assign(env,{GetGame:()=>game,ChimeraCharacter:{Cast:x=>x},
    AROR_GameFlowComponent:{Cast:x=>x},AROR_GarrisonComponent:{Cast:x=>x},
    Math:{...env.Math,floor:Math.floor}});
  const playerNear=compile(sources.manager,'PlayerNear');
  const playerClearance=compile(sources.garrison,'HasPlayerClearance');
  playerPosition=[0,0,0];
  check(playerClearance.call({m_fCrewPlayerClearance:3},[4,0,0],4,6),'standing on the flag does not suppress its four-metre crew staging ring');
  check(!playerClearance.call({m_fCrewPlayerClearance:3},[2,0,0],4,6),'crew still cannot spawn directly against a player');
  check(!playerClearance.call({m_fCrewPlayerClearance:3},[4,0,0],0,6),'infantry pads retain the normal player exclusion');
  playerPosition=[1001,0,0];
  const allowedCentre=compile(sources.garrison,'IsDefenceCenterAllowed');
  const anchored={m_bFixedDefenceAnchor:true,m_Location:{GetCenter:()=>[0,0,0],GetRadius:()=>250}};
  check(allowedCentre.call(anchored,[100,0,0],24),'anchored compact defence fits inside canonical capture circle');
  check(!allowedCentre.call(anchored,[230,0,0],24),'anchored perimeter cannot leave the capture circle');
  check(!allowedCentre.call(anchored,[700,0,0],26),'distant resource clearing cannot replace a fixed strategic anchor');
  check(allowedCentre.call({m_bFixedDefenceAnchor:false},[700,0,0],26),'ordinary outpost relocation is not constrained by its old capture circle');
  check(!playerNear.call({m_fActivationRadius:1000},centre),'outside 1km does not activate');
  playerPosition=[1000,0,0];check(playerNear.call({m_fActivationRadius:1000},centre),'1km boundary activates');
  playerPosition=[0,0,0];check(playerNear.call({m_fActivationRadius:1000},centre),'teleport directly onto flag activates without a boundary event');
  playerDead=true;check(!playerNear.call({m_fActivationRadius:1000},centre),'dead players do not activate');playerDead=false;
  let assets=0,groups=0,lastClearance=0,lastGroupTime=-1,terrainFits=true,active=false,reserved=9,retirementTime=-1;
  const teleportSite={...centre,GetControllingRole:()=>1,GetOwner:()=>({FindComponent:()=>defence}),GetId:()=>'test'};
  const defence={IsEligible:()=>true,IsGarrisonActive:()=>active,Observe_S:()=>{},CanRetire:()=>false,
    OutsideLongEnough:(outside,now)=>{retirementTime=now;return false;},RefreshPending_S:()=>{},
    HasRoster:()=>true,GetRosterRole:()=>1,GetLocation:()=>teleportSite,ReservedPersonnel:()=>reserved,
    HasSuitableDefenceTerrain_S:()=>terrainFits,
    SpawnNextAsset_S:(binding,clearance)=>{assets++;lastClearance=clearance;return false;},
    SpawnNextGroup_S:(now,clearance)=>{groups++;lastClearance=clearance;lastGroupTime=now;return true;}};
  env.AROR_LocationManagerComponent={GetInstance:()=>({GetLocations:out=>out.push(teleportSite)})};
  const managerTick=compile(sources.manager,'Tick');
  const managerState={m_bEnabled:true,m_iElapsedMilliseconds:0,m_iCursor:0,m_fActivationRadius:1000,
    m_fPlayerSpawnClearance:6,m_fRetirementRadius:2500,m_iRetirementDelay:120,m_iMaxPersonnel:96,
    PlayerNear:playerNear,FactionFor:()=>({}),GetOwner:()=>({FindComponent:()=>({IsPlaythroughReady:()=>ready})})};
  // Policy is compiled below before ticking the manager.
  for(const name of ['DefenceLevel','InfantryTeams','StaticCount','MannedCount','IsEnemyRole'])env.AROR_GarrisonPolicy[name]=compile(sources.policy,name);
  managerTick.call(managerState);
  check(assets===1&&groups===1&&lastClearance===6,'teleport onto flag admits defenders with local clearance instead of a 150m exclusion');
  for(let i=0;i<3;i++)managerTick.call(managerState);
  check(lastGroupTime===1,'four quarter-second ticks advance queued group time by one second');
  active=true;managerTick.call(managerState);check(retirementTime===1,'retirement still receives seconds after faster polling');active=false;
  terrainFits=false;const started=assets;managerTick.call(managerState);check(assets===started,'teleport does not bypass unsuitable terrain');terrainFits=true;
  reserved=97;managerTick.call(managerState);check(assets===started,'teleport does not bypass capacity');reserved=9;
  playerPosition=[1001,0,0];managerTick.call(managerState);check(assets===started,'moving beyond 1km stops new construction');playerPosition=[0,0,0];
  ready=false;managerTick.call(managerState);check(assets===started,'teleport cannot spawn before campaign setup');ready=true;
  server=false;managerTick.call(managerState);check(assets===started,'client cannot activate after teleport');server=true;
  for(const roll of [0,1,2,99]){
    check(env.AROR_GarrisonPolicy.DefenceLevel(types.OUTPOST,roll)===1,'outposts cannot roll vehicle recipes');
    check(env.AROR_GarrisonPolicy.DefenceLevel(types.AIRPORT,roll)===4,'airbases always heavy');
    check(env.AROR_GarrisonPolicy.DefenceLevel(types.MILITARY_BASE,roll)===3,'military outpost tier');
    check(env.AROR_GarrisonPolicy.DefenceLevel(types.TOWN,roll)===0,'no town garrison recipe');
  }
  check([0,1,2].map(r=>env.AROR_GarrisonPolicy.DefenceLevel(types.RESOURCES,r)).join(',')==='1,2,3','resources roll three stored defence levels');
  const faction={m_eRole:1,m_sFactionKey:'AROR_OCCUPIERS',m_sAssetFactionKey:'US',m_sInfantry:'rifle',m_sStaticMG:'mg',m_sCrew:'crew',m_sParkedArmour:'lav',m_aMannedVehicles:['lav','humvee']};
  const add=compile(sources.garrison,'AddSlot'),reserve=compile(sources.garrison,'ReservedPersonnel'),generate=compile(sources.garrison,'Generate_S');
  function roster(type,override=0){
    const state={m_aSlots:[],m_bGenerated:false,m_iDefenceLevel:override,IsEligible:()=>true,
      m_Location:{GetControllingRole:()=>1,GetLocationType:()=>type,GetId:()=>'site'},AddSlot:add,ReservedPersonnel:reserve};
    generate.call(state,faction);return state;
  }
  const outpost=roster(types.OUTPOST,4);
  check(outpost.m_aSlots.filter(s=>s.m_iKind===2||s.m_iKind===3).length===0,'even strong outpost overrides never add vehicles');
  const military=roster(types.MILITARY_BASE);
  check(military.m_aSlots.filter(s=>s.m_iKind===2).length===1,'military outpost includes parked unmanned armour');
  check(military.m_aSlots.filter(s=>s.m_iKind===3).length===1,'military outpost includes one random manned hull');
  const airport=roster(types.AIRPORT,1);
  check(airport.m_iLevel===4&&airport.m_aSlots.filter(s=>s.m_iKind===3).length===3,'airbase stays large under low override');
  check(reserve.call(airport)===48,'airbase reserves all infantry, static and possible hull crew before spawning');
  const before=airport.m_aSlots.length;check(!generate.call(airport,faction)&&airport.m_aSlots.length===before,'active recipe cannot reroll');
  server=false;check(!generate.call({...military,m_bGenerated:false},faction),'client cannot generate roster');server=true;

  const advance=compile(sources.support,'ApplyRegionalSupport_S');
  function support(r,o,i){return {m_fResistance:r,m_fOccupiers:o,m_fInvaders:i,m_Location:{IsControlFixed:()=>false},
    SetValues(a,r,b,o,c,i){this.m_fResistance=r;this.m_fOccupiers=o;this.m_fInvaders=i;this.UpdateControl_S();},UpdateControl_S(){this.checked=(this.checked||0)+1;}};}
  const town=support(0,100,0);
  for(let n=0;n<26;n++)advance.call(town,1,0,0,2);
  check(town.m_fResistance===52&&town.m_fOccupiers===48&&town.m_fInvaders===0,'regional majority emerges gradually without donating support to absent third role');
  const mixed=support(10,60,30);advance.call(mixed,1,1,1,2);
  check(Math.abs(mixed.m_fResistance+mixed.m_fOccupiers+mixed.m_fInvaders-100)<1e-9,'three-role distribution stays normalized');
  check(Math.max(Math.abs(mixed.m_fResistance-10),Math.abs(mixed.m_fOccupiers-60),Math.abs(mixed.m_fInvaders-30))<=2.000001,'step caps every role');
  const untouched=support(0,100,0);advance.call(untouched,0,0,0,2);check(untouched.m_fOccupiers===100&&!untouched.checked,'no installations preserves support');
  untouched.m_Location.IsControlFixed=()=>true;advance.call(untouched,1,0,0,2);check(untouched.m_fResistance===0,'fixed control preserves support');
  const majority=support(100,0,0);advance.call(majority,1,0,0,2);check(majority.checked===1,'existing majority rechecks expired flip cooldown');
  server=false;advance.call(town,0,1,0,2);check(town.m_fResistance===52,'clients cannot influence support');server=true;

  const region=compile(sources.region,'RegionFor');
  const loc=(id,type,pos,override='')=>({GetId:()=>id,GetLocationType:()=>type,GetCenter:()=>pos,GetSupportRegionId:()=>override});
  const a=loc('a',types.TOWN,[-10,0,0]),b=loc('b',types.CITY,[10,0,0]),site=loc('site',types.OUTPOST,[0,0,0]);
  check(region(site,[b,a])==='a'&&region(site,[a,b])==='a','nearest-region ties use stable IDs, independent of registration');
  check(region(loc('site',types.OUTPOST,[0,0,0],'manual'),[a,b])==='manual','military authored region overrides distance');
  check(region(a,[a,b])==='a','settlement defaults to own stable region ID');
  const shared=loc('a',types.TOWN,[-10,0,0],'larger');check(region(site,[b,shared])==='larger','nearest settlement propagates larger shared region');
  check(region(site,[])==='','missing settlements invent no region');

  const outside=compile(sources.garrison,'OutsideLongEnough'),state={m_iOutsideSince:-1};
  check(!outside.call(state,true,100,120),'leaving radius starts grace');
  check(!outside.call(state,true,219,120)&&outside.call(state,true,220,120),'retirement requires entire grace interval');
  check(!outside.call(state,false,221,120)&&state.m_iOutsideSince===-1,'returning player resets grace');
  check(!outside.call(state,true,230,120),'new exit cannot inherit old grace');

  // Observe actual recorded assets: a retired living entity is not a death; missing physical assets are losses.
  Object.assign(env,{DamageManagerComponent:{Cast:x=>x},ChimeraCharacter:{Cast:e=>e&&e.character?e:null}});
  const observe=compile(sources.garrison,'Observe_S');
  let destroyed=false,health=0.6,foreign=false;
  const entity={FindComponent:()=>({IsDestroyed:()=>destroyed,GetHealthScaled:()=>health})};
  const survivor={m_bLost:false,m_bSpawned:true,m_Entity:entity,m_fHealth:1};
  const missing={m_bLost:false,m_bSpawned:true,m_Entity:null,m_fHealth:1};
  const virtual={m_bLost:false,m_bSpawned:false,m_Entity:null,m_fHealth:1};
  const garrison={m_aGroups:[],m_aSlots:[survivor,missing,virtual],IsPlayerOrForeignAsset:()=>foreign,HasDormantReserve:()=>false};
  observe.call(garrison,false);check(survivor.m_fHealth===0.6&&!survivor.m_bLost,'damage sampled without declaring survivor dead');
  check(missing.m_bLost&&!virtual.m_bLost,'only formerly physical missing assets become losses');
  health=1;observe.call(garrison,false);check(survivor.m_fHealth===0.6,'virtualisation cannot manufacture healing');
  destroyed=true;observe.call(garrison,false);check(survivor.m_bLost,'confirmed destruction retained');
  destroyed=false;survivor.m_bLost=false;foreign=true;observe.call(garrison,false);
  check(survivor.m_bLost&&survivor.m_Entity===null,'captured/player assets detach from cleanup and cannot respawn');
	missing.m_bLost=false;garrison.HasDormantReserve=()=>true;observe.call(garrison,false);
	check(!missing.m_bLost,'native dormant reserve is never treated as a missing combat casualty');
	const pending=compile(sources.garrison,'IsDefencePending');
	server=false;check(pending.call({m_bPendingDefense:false})===false,'client cleared state comes from replication rather than ungenerated local roster');
	check(pending.call({m_bPendingDefense:true})===true,'client honours replicated deployment lock');server=true;
  const established=compile(sources.garrison,'HasEstablishedDefence_S');
  const fallen={m_iKind:0,m_iGroup:0,m_bLost:true,m_bSpawned:true,m_Entity:null};
  const blocked={m_iKind:1,m_iGroup:1,m_bLost:false,m_bSpawned:false,m_Entity:null,m_iVehicle:-1,m_iPlacementFailureSince:-1};
  const guard={IsEligible:()=>true,m_bGenerated:true,m_eRosterRole:1,m_Location:{GetControllingRole:()=>1,GetId:()=>'site'},
    m_aSlots:[fallen,blocked],m_aGroups:[],HasEstablishedDefence_S:established};
  check(!pending.call(guard),'cleared physical defence permits capture despite a blocked unqueued MG');
  const queuedCrew={m_iKind:4,m_bLost:false,m_bSpawned:false,m_iVehicle:1,m_iGroup:1};
  guard.m_aSlots.push(queuedCrew);guard.m_aGroups=[{m_iId:1,m_aSlots:[2]}];
  check(pending.call(guard),'unresolved native personnel queue still protects against premature capture');
  queuedCrew.m_bSpawned=true;check(!pending.call(guard),'resolved queue restores physical-hostile capture rule');
  guard.m_aGroups=[];guard.m_aSlots=[{...fallen,m_bLost:false,m_bSpawned:false}];
  check(pending.call(guard),'capture cannot race an entirely unspawned initial defence');
  let placementTime=0;
  env.GetGame=()=>({GetWorld:()=>({GetWorldTime:()=>placementTime})});
  const placementFailure=compile(sources.garrison,'ReportPlacementFailure');
  const placementState={m_aSlots:[blocked,{...queuedCrew,m_bSpawned:false,m_iVehicle:0}],m_aGroups:[],m_iPlacementRetrySeconds:30,
    m_Location:{GetId:()=>'site'},ReportFailure:()=>{}};
  placementFailure.call(placementState,0,'blocked');placementTime=29999;placementFailure.call(placementState,0,'blocked');
  check(!blocked.m_bPlacementUnavailable,'failed placement remains retryable during its bounded window');
  placementTime=30000;placementFailure.call(placementState,0,'blocked');
  check(blocked.m_bPlacementUnavailable&&placementState.m_aSlots[1].m_bPlacementUnavailable,'failed hull/MG and its unqueued crew omit together');
  check(!blocked.m_bLost&&!placementState.m_aSlots[1].m_bLost,'omitting a never-spawned asset creates no casualties');
  const queuedSlot={...blocked,m_bPlacementUnavailable:false,m_iPlacementFailureSince:0};
  placementState.m_aSlots=[queuedSlot];placementState.m_aGroups=[{m_iId:1}];placementTime=60000;
  placementFailure.call(placementState,0,'blocked');check(!queuedSlot.m_bPlacementUnavailable,'native queued personnel cannot be discarded by pad timeout');
  placementState.m_aGroups=[];queuedSlot.m_bSpawned=true;placementFailure.call(placementState,0,'blocked');
  check(!queuedSlot.m_bPlacementUnavailable,'live assets cannot be omitted by placement timeout');
  queuedSlot.m_bSpawned=false;server=false;placementFailure.call(placementState,0,'blocked');
  check(!queuedSlot.m_bPlacementUnavailable,'clients cannot omit defence assets');server=true;
  check(sources.location.includes('support.GetSupport(role) <= 50'),'canonical town setter requires majority, including direct external calls');
  const deploymentReason=compile(sources.garrison,'GetDefenceCaptureBlockReason');
  const deployment={IsDefencePending:()=>true,m_bDefenceTerrainChecked:true,m_bDefenceTerrainFits:false,m_bDeploymentDeferred:true};
  check(deploymentReason.call(deployment)==='Defence site unsuitable - relocation required','rejected terrain is not labelled as living hostiles');
  deployment.m_bDefenceTerrainFits=true;check(deploymentReason.call(deployment)==='Defence deployment delayed','obstructed placement has distinct feedback');
  deployment.m_bDeploymentDeferred=false;check(deploymentReason.call(deployment)==='Defenders awaiting deployment','queued defenders have explicit feedback');
  deployment.IsDefencePending=()=>false;check(deploymentReason.call(deployment)==='','cleared roster removes deployment capture gate');
  server=false;check(deploymentReason.call({m_sPendingDefenseReason:'Defence deployment delayed'})==='Defence deployment delayed','client uses replicated reason');server=true;
  let bumps=0;env.Replication.BumpMe=()=>bumps++;
  const status={m_bPendingDefense:true,m_sPendingDefenseReason:'Defenders awaiting deployment',IsDefencePending:()=>true,GetDefenceCaptureBlockReason:()=>'Defence site unsuitable - relocation required'};
  compile(sources.garrison,'RefreshPending_S').call(status);check(bumps===1&&status.m_sPendingDefenseReason.startsWith('Defence site unsuitable'),'reason-only changes replicate while pending remains true');
  let livingHostile=false;Object.assign(env,{AROR_HostilesQuery:{HasHostilesInRadius:()=>livingHostile},GetGame:()=>({GetWorld:()=>({GetWorldTime:()=>10000})})});
  const physical=compile(sources.capture,'HasHostiles'),captureState={m_fLastHostileCheckTime:0,m_bHostilesPresent:false,GetLocation:()=>({GetCenter:()=>[0,0,0],GetRadius:()=>100})};
  check(!physical.call(captureState,{},false),'virtual/unspawned roster is not evidence of a physical hostile');
  livingHostile=true;check(physical.call(captureState,{},false),'living hostile query still blocks capture');
  check(sources.capture.includes('garrison.GetDefenceCaptureBlockReason()'),'deployment remains a separate authoritative capture check');
	const dormant=compile(sources.garrison,'HasDormantReserve');
	const sleeping={m_aGroups:[{m_iId:2,m_Group:{IsDormant:()=>true,GetDormantAliveCount:()=>3}}]};
	check(dormant.call(sleeping,{m_iGroup:2,m_iKind:0}),'native dormant infantry keeps its reserve');
	check(!dormant.call(sleeping,{m_iGroup:2,m_iKind:3}),'dormant crew never excuses a missing hull');
	Object.assign(env,{AROR_GarrisonComponent:{Cast:x=>x},ESerializeResult:{OK:1,DEFAULT:0}});
	const write=compile(sources.serializer,'Serialize'),read=compile(sources.serializer,'Deserialize');
	const saved={};let observed=false,restored=null;
	const snapshot=military.m_aSlots;Object.assign(snapshot[0],{m_bLost:true,m_fHealth:0,m_bPlaced:true,m_vPosition:[1,2,3],m_vAngles:[0,45,0]});
	const original={HasRoster:()=>true,Observe_S:orders=>{check(orders===false,'saving never mounts/orders/spawns actors');observed=true;},
	  GetSlots:out=>out.push(...snapshot),GetRosterRole:()=>1,GetRosterFaction:()=>faction.m_sFactionKey,GetAssetFaction:()=>faction.m_sAssetFactionKey,GetLevel:()=>3};
	check(write(null,original,{WriteValue:(key,value)=>saved[key]=value})===1&&observed,'versioned roster snapshot writes');
	const loaded={RestoreRoster_S:(...args)=>restored=args},context={ReadValue:(key,set)=>{if(!(key in saved))return false;set(saved[key]);return true;}};
	check(read(null,loaded,context)&&restored[4].length===snapshot.length,'saved recipe restores all slots');
	check(restored[4][0].m_bLost&&restored[4][0].m_fHealth===0&&restored[4][0].m_vPosition[1]===2,'casualty, damage and placement survive round trip');
	saved.version=99;restored=null;check(!read(null,loaded,context)&&!restored,'unknown save version cannot partially replace state');saved.version=1;
	check(read(null,loaded,context)&&!restored[4][0].m_bPlaced&&restored[4][0].m_bLost&&restored[4][0].m_fHealth===0,'v1 migration keeps losses/damage while invalidating scattered placement');
	saved.slot0_kind=4;saved.slot0_vehicle=999;check(!read(null,loaded,context),'invalid crew parent rejected before restoration');
  return {assertions,note:'Mocked native services; Workbench compile, crew seating, placement, replication and save/load remain required.'};
}
if(typeof require==='function'&&typeof module!=='undefined'&&require.main===module){
  const fs=require('fs'),path=require('path'),root=path.join(__dirname,'../AROpenResistance/Open Resistance/Scripts/Game');
  const sources={};for(const [key,name]of Object.entries({manager:'Garrisons/AROR_GarrisonManagerComponent',policy:'Garrisons/AROR_GarrisonPolicy',garrison:'Garrisons/AROR_GarrisonComponent',serializer:'Garrisons/AROR_GarrisonComponentSerializer',support:'Settlements/AROR_SettlementComponent',region:'Settlements/AROR_RegionalSupportComponent',location:'Locations/AROR_LocationComponent',capture:'Capture/AROR_CaptureLocationAction'}))sources[key]=fs.readFileSync(path.join(root,name+'.c'),'utf8');
  console.log(JSON.stringify(runGarrisonsAndSupportTests(sources)));
}
