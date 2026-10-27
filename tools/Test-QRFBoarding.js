// Actual deployment seating/recovery methods with paused native actions mocked.
const fs=require('fs'),path=require('path');
const source=fs.readFileSync(path.join(__dirname,'../AROpenResistance/Open Resistance/Scripts/Game/Operations/AROR_GroundOperationForce.c'),'utf8');
let assertions=0,server=true;
const check=(ok,message)=>{assertions++;if(!ok)throw Error(message);};
for(const [name,fn]of Object.entries({Count(){return this.length;},Insert(x){this.push(x);},Contains(x){return this.includes(x);}}))Object.defineProperty(Array.prototype,name,{value:fn,configurable:true});
const env={Replication:{IsServer:()=>server},GetGame:()=>({GetPlayerManager:()=>({GetPlayerIdFromControlledEntity:x=>x?.player?1:0})}),
  AROR_EForceType:{AIRBORNE:3,MOTORIZED:0},ECloseDoorAfterActions:{CLOSE_DOOR:1},
  string:{Format:(s,...a)=>s.replace(/%(\d+)/g,(_,i)=>a[i-1])}};
for(const name of ['BaseCompartmentManagerComponent','SCR_AIVehicleUsageComponent','CompartmentAccessComponent'])env[name]={Cast:x=>x};
env.ChimeraCharacter={Cast:x=>x?.character?x:null};
env.TurretCompartmentSlot={Cast:x=>x?.kind==='turret'?x:null};env.CargoCompartmentSlot={Cast:x=>x?.kind==='cargo'?x:null};
const methods=['Seat','IsMountComplete','ReseatMissing_S','ReseatMember','RecoverStaleBoarding_S','IsSeatedInOwnVehicle'];
function compile(name){
  const m=source.match(new RegExp('^\\t[^\\r\\n]*\\b'+name+'\\(([^\\r\\n]*)\\)\\r?\\n\\t\\{([\\s\\S]*?)^\\t\\}', 'm'));
  if(!m)throw Error('Missing '+name);
  const args=m[1].replace(/array<\w+>\s+|\b(?:IEntity|bool|BaseCompartmentSlot|CompartmentAccessComponent)\s+/g,'');
  let body=m[2].replace(/foreach \(\w+ (\w+) : ([^)]+)\)/g,'for(const $1 of $2)')
    .replace(/array<\w+> (\w+) = \{([^;]*)\};/g,'let $1 = [$2];')
    .replace(/int (\w+), (\w+), (\w+);/g,'let $1=0,$2=0,$3=0;')
    .replace(/\b(?:IEntity|bool|int|float|string|Physics|ChimeraCharacter|BaseCompartmentSlot|BaseCompartmentManagerComponent|SCR_AIVehicleUsageComponent|CompartmentAccessComponent)\s+(\w+)/g,'let $1')
    .replace(/let (\w+);/g,'let $1=0;').replace(/(?<![.\w])m_\w+/g,x=>'this.'+x);
  for(const n of methods)body=body.replace(new RegExp('(?<![.\\w])'+n+'\\(','g'),'this.'+n+'(');
  return new Function(...Object.keys(env),'return function('+args+'){'+body+'}')( ...Object.values(env));
}
const compiled=Object.fromEntries(methods.map(n=>[n,compile(n)]));
function fixture(crewCount=1,passengerCount=4){
  const all=[],calls=[],resets=[];
  const vehicle={speed:0,FindComponent(type){return type===env.BaseCompartmentManagerComponent?{GetCompartments:a=>a.push(...slots)}:usage;},GetPhysics(){return {GetVelocity:()=>({Length:()=>this.speed})};}};
  function seat(kind){return {kind,occupant:null,reserved:null,accessible:true,GetOccupant(){return this.occupant;},IsOccupied(){return !!this.occupant;},
    IsCompartmentAccessible(){return this.accessible;},IsReserved(){return !!this.reserved;},IsReservedBy(x){return this.reserved===x;},SetReserved(x){this.reserved=x;},GetVehicle:()=>vehicle};}
  const slots=[seat('driver'),...Array.from({length:crewCount-1},()=>seat('turret')),...Array.from({length:passengerCount},()=>seat('cargo'))];
  const usage={GetPilotCompartmentSlot:()=>slots[0]};
  function actor(){
    const a={character:true,dead:false,GetCharacterController(){return {IsDead:()=>this.dead};},FindComponent:()=>a.access};
    a.access={entering:true,exiting:false,inside:false,current:null,allow:true,
      IsGettingIn(){return this.entering;},IsGettingOut(){return this.exiting;},IsInCompartment(){return this.inside;},GetCompartment(){return this.current;},
      InterruptVehicleActionQueue(...args){resets.push({actor:a,args});this.entering=false;},
      GetInVehicle(v,slot,teleport,door,close,paused){calls.push({actor:a,slot,paused});if(!this.allow)return false;
        // Outside the simulation bubble, an ordinary queued action cannot execute.
        if(!paused){this.entering=true;return true;}this.current=slot;this.inside=true;this.entering=false;slot.occupant=a;return true;}};
    all.push(a);return a;
  }
  const crew=Array.from({length:crewCount},actor),passengers=Array.from({length:passengerCount},actor);
  const group=members=>({players:0,GetPlayerCount(){return this.players;},GetAgents:out=>out.push(...members.map(x=>({GetControlledEntity:()=>x})))});
  all.forEach((a,i)=>slots[i].reserved=a);
  const force={...compiled,m_Vehicle:vehicle,m_Profile:{m_eType:0},m_CrewGroup:group(crew),m_Group:group(passengers),
    m_aMembers:all,m_aPassengers:passengers,m_bMountRequested:true,m_bMounted:false,m_bDismountRequested:false,m_bDismounted:false,
    m_iSeatAttempts:0,m_iSeatQueueResets:0,m_iSeatBusy:0};
  return {force,vehicle,crew,passengers,slots,all,calls,resets};
}
let f=fixture();
check(!f.force.IsMountComplete(),'missing native occupancy cannot activate deployment');
let report=f.force.ReseatMissing_S(false);
check(!f.calls.length&&!f.resets.length&&report.includes('busy=5'),'normal grace does not interrupt pending actions');
report=f.force.ReseatMissing_S(true);
check(f.resets.length===5&&f.calls.length===5,'stuck driver and all passengers receive recovery and a fresh seat request');
check(f.calls.every(x=>x.paused===true),'initial teleports execute when simulation is paused');
check(f.resets.every(x=>x.args.every(v=>v===true)),'interrupt current action, force and replicate native queue reset');
check(f.force.IsMountComplete(),'activation becomes possible only after actual driver/passenger occupancy');
check(report.includes('accepted=5 attempted=5 queueResets=5 busy=0'),'retry diagnostics distinguish requests from acceptance');
f=fixture(3,4);f.force.ReseatMissing_S(true);
check(f.force.IsMountComplete()&&f.calls.length===7,'IFV crew and infantry all recover to distinct seats');
check(new Set(f.calls.map(x=>x.slot)).size===7,'one occupant per seat');
f=fixture();f.crew[0].access.allow=false;report=f.force.ReseatMissing_S(true);
check(report.includes('accepted=4 attempted=5'),'native rejection distinguishable from retry being skipped');
check(!f.force.IsMountComplete(),'accepted passengers cannot hide an absent driver');
f=fixture();f.crew[0].access.entering=false;check(f.force.Seat(f.crew[0],f.slots[0])&&f.calls[0].paused,'first placement also uses paused execution');
f=fixture();f.force.m_bMounted=true;f.force.Seat(f.crew[0],f.slots[0]);check(f.calls[0].paused===false,'live-operation reseating does not force paused execution');
for(const [label,mutate]of [
  ['client',f=>server=false],['already mounted',f=>f.force.m_bMounted=true],['dismounting',f=>f.force.m_bDismountRequested=true],
  ['already dismounted',f=>f.force.m_bDismounted=true],['no initial request',f=>f.force.m_bMountRequested=false],
  ['aircraft',f=>f.force.m_Profile.m_eType=3],['player',f=>f.crew[0].player=true],['player-controlled crew group',f=>f.force.m_CrewGroup.players=1],
  ['player-controlled infantry group',f=>f.force.m_Group.players=1],['dead actor',f=>f.crew[0].dead=true],
  ['foreign actor',f=>f.force.m_aMembers=[]],['moving vehicle',f=>f.vehicle.speed=2],['already seated',f=>f.crew[0].access.inside=true],
  ['exiting',f=>f.crew[0].access.exiting=true],['idle actor',f=>f.crew[0].access.entering=false],
  ['other vehicle',f=>f.crew[0].access.current={GetVehicle:()=>({})}]]){
  server=true;f=fixture();mutate(f);
  check(!f.force.RecoverStaleBoarding_S(f.crew[0],f.crew[0].access),'do not interrupt '+label);
  check(f.resets.length===0,'native queue untouched for '+label);
}
server=true;f=fixture();const other={};f.slots[1].reserved=other;
f.force.RecoverStaleBoarding_S(f.crew[0],f.crew[0].access);
check(f.slots[0].reserved===null&&f.slots[1].reserved===other,'only actor-owned empty claims cleared');
f=fixture();f.slots[0].occupant=other;
check(!f.force.Seat(f.crew[0],f.slots[0])&&f.slots[0].occupant===other,'real occupants never displaced');
f=fixture();f.crew[0].access.entering=false;f.slots[0].reserved=other;
check(!f.force.ReseatMember(f.crew[0],[f.slots[0]],null),'another actor reservation is not stolen');
f=fixture();f.crew[0].player=true;
check(!f.force.Seat(f.crew[0],f.slots[0])&&!f.calls.length,'player never teleported into deployment seat');
console.log(JSON.stringify({assertions,note:'Native paused simulation/queue cancellation must be verified in Workbench Play.'}));
