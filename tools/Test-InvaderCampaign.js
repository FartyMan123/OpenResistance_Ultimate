/* Actual-source territorial simulation. Native game lifecycle, income timing, persistence and UI remain mocked.
 * Run: node tools/Test-InvaderCampaign.js
 */
function runInvaderCampaignTests(sources, catalogue) {
  let assertions=0, server=true, now=0, campaignReady=true, rolls=[], reports=0, bumps=0;
  const check=(ok,msg)=>{assertions++;if(!ok)throw Error(msg);};
  for(const [name,fn] of Object.entries({Count(){return this.length;},Insert(x){this.push(x);},Clear(){this.length=0;},Copy(a){this.splice(0,this.length,...a);}}))Object.defineProperty(Array.prototype,name,{value:fn,configurable:true});
  Object.defineProperty(String.prototype,'Compare',{value(other){return this<other?-1:this>other?1:0;},configurable:true});
  const role={RESISTANCE:0,OCCUPIERS:1,INVADERS:2},type={TOWN:0,HQ:1,AIRPORT:2,CITY:3,FACTORY:4,MILITARY_BASE:5,PORT:6,RESOURCES:7,OUTPOST:8,SUPPORT_CORRIDOR:9};
  let seed=12345;const random=()=>{seed=(1664525*seed+1013904223)>>>0;return seed/4294967296;};
  const bank={balances:{1:100000,2:10000},payments:[],GetFactionFunds(r){return this.balances[r]??0;},
    AddFactionFunds_S(r,n){if(server)this.balances[r]+=n;},SpendFactionFunds_S(r,n){if(!server||n<0||this.balances[r]<n)return false;this.balances[r]-=n;this.payments.push([r,n]);return true;}};
  const reserves={1:0,2:0},debt={1:0,2:0},finance={Available:r=>debt[r]>0?0:Math.max(0,bank.GetFactionFunds(r)-reserves[r])};
  let war=2,aggression=10,physical=[],pending=false,allSites=[];
  const operations={GetWarLevel:()=>war,GetAggressionPercent:()=>aggression,GetQRFFinance:()=>finance,GetOperations:a=>a.Copy(physical),HasPendingQRFThreat:()=>pending};
  const env={AROR_EFactionRole:role,AROR_ELocationType:type,ESerializeResult:{OK:1},float:{MAX:Infinity},
    Replication:{IsServer:()=>server,BumpMe:()=>bumps++},Math:{Min:Math.min,Max:Math.max,Round:Math.round,Floor:Math.floor,Ceil:Math.ceil,Clamp:(x,a,b)=>Math.max(a,Math.min(b,x)),RandomInt:(a,b)=>rolls.length?rolls.shift():Math.floor(a+random()*(b-a))},
    vector:{DistanceXZ:(a,b)=>Math.hypot(a[0]-b[0],a[2]-b[2])},Print(){},LogLevel:{NORMAL:0,ERROR:1},string:{Format:()=>''},
    AROR_GarrisonPolicy:{IsEnemyRole:r=>r===1||r===2},AROR_GarrisonComponent:{Cast:x=>x},AROR_FactionRolesComponent:{GetInstance:()=>({GetFaction:r=>({role:r,IsFactionEnemy:other=>r!==other.role})})},
    AROR_GameFlowComponent:{GetInstance:()=>({IsPlaythroughReady:()=>campaignReady})},AROR_OperationManagerComponent:{GetInstance:()=>operations},AROR_EconomyComponent:{GetInstance:()=>bank},
    GetGame:()=>({GetWorld:()=>({GetWorldTime:()=>now}),GetPlayerManager:()=>({GetPlayers(){},GetPlayerControlledEntity:()=>null})})};
  const names=['TargetShare','Funding','InvaderAttackChance','BattleChance','Cost','IsAdvanceActive','HasOpeningGrant','IsQuarantined','SuppressAICaptureQRF_S','GetExternalSupportBonus','RemainingDelay','RestoreCampaign_S','Quarantine_S',
    'OnEvaluation','Survey_S','IsDistant','SelectTarget','Industry','ResolveBattle_S'];
  function compile(src,name){
    const m=src.match(new RegExp('^\\t[^\\r\\n]*\\b'+name+'\\(([^\\r\\n]*)\\)\\s*\\{([^\\r\\n]*)\\}', 'm'))||src.match(new RegExp('^\\t[^\\r\\n]*\\b'+name+'\\(([^\\r\\n]*)\\)\\r?\\n\\t\\{([\\s\\S]*?)^\\t\\}', 'm'));
    if(!m)throw Error('Missing '+name);
    const args=m[1].replace(/\b(?:notnull |const )?\w+(?:<[^>]+>)?\s+(\w+)/g,'$1');
    let body=m[2].replace(/\barray<[^>]+>\s+(\w+)\s*=\s*\{\s*\};/g,'let $1=[];')
      .replace(/foreach\s*\(\w+\s+(\w+)\s*:\s*([^\)]+)\)/g,'for(let $1 of $2)')
      .replace(/\b(?:bool|int|float|string|vector|IEntity|Faction|AROR_\w+)\s+(\w+)/g,'let $1')
      .replace(/let (\w+);/g,'let $1=0;').replace(/(?<![.\w])[ms]_\w+/g,x=>'this.'+x)
      .replace(/context\.ReadValue\("([^"]+)", (\w+)\)/g,'(($2=context.values["$1"])!==undefined)');
    for(const n of names)body=body.replace(new RegExp('(?<![.\\w])'+n+'\\(','g'),'this.'+n+'(');
    return new Function(...Object.keys(env),'return function('+args+'){'+body+'}')( ...Object.values(env));
  }
  const policy={};env.AROR_InvaderCampaignPolicy=policy;
  for(const n of names.slice(0,5))policy[n]=compile(sources.policy,n);
  const campaign={m_bInvaderCampaign:true,m_bSeeded:false,m_bQuarantined:false,m_iNextBattleAt:0,m_iRestoredDelay:-1,m_iInitialDelay:120,m_iBattleInterval:300,m_iOpeningGrant:20000,
    m_sCorridorId:'corridor',m_sForwardBaseId:'base',m_iBaselineShare:40,m_iBattleCost:1000,m_fFrontierReach:3500,m_fPlayerClearance:3000,m_iExternalSupport:1500,m_iFundingPerWar:750,m_iFundingPerAggression:30,
    m_aSites:[],m_aPlayers:[],m_aOperations:[],m_Commanders:{IsCampaignActive:()=>true},m_Operations:operations,m_Economy:bank,Cast:x=>x};
  env.AROR_InvaderCampaignComponent=campaign;campaign.s_Instance=campaign;campaign.GetInstance=()=>campaign;
  const registry={GetLocations:a=>a.Copy(allSites),GetLocationById:id=>allSites.find(s=>s.id===id)};env.AROR_LocationManagerComponent={GetInstance:()=>registry};
  for(const n of names.slice(5))campaign[n]=compile(sources.component,n);
  const makeSite=(id,owner,pos,t=type.OUTPOST)=>({id,owner,pos,t,fixed:false,active:false,started:false,locked:false,
    GetId(){return this.id;},GetControllingRole(){return this.owner;},GetCenter(){return this.pos;},GetRadius:()=>100,GetLocationType(){return this.t;},IsControlFixed(){return this.fixed;},
    CanBeTakenBy(r){return !this.fixed&&!this.locked&&r!==this.owner;},GetOwner(){return {FindComponent:()=>({IsGarrisonActive:()=>this.active,HasDeploymentStarted:()=>this.started})};},
    SetControllingRole_S(r){if(!server||!this.CanBeTakenBy(r))return false;this.owner=r;return true;}});
  const corridor=makeSite('corridor',2,[11600,0,800],type.SUPPORT_CORRIDOR);corridor.fixed=true;
  const base=makeSite('base',2,[8732,100,3017]),near=makeSite('near',1,[8606,100,3389]),far=makeSite('far',1,[7802,100,4892]);
  allSites=[corridor,base,near,far];
  check(policy.BattleChance(700,700,0,0)===50,'equal resources and industry produce 50/50 engagements');
  check(policy.BattleChance(1000,0,0,0)>50&&policy.BattleChance(1000,1000,8,0)>50,'funding and industry influence battle strength');
  check(policy.TargetShare(40,1,0)===40&&policy.TargetShare(40,2,10)===40,'calm baseline targets two fifths');
  check(policy.TargetShare(40,5,100)>40&&policy.TargetShare(80,5,100)===80,'escalation expands bounded territorial ambition');
  check(policy.Funding(1500,750,30,2,10)===1800&&policy.Funding(1500,750,30,5,100)===6750,'war/aggression scale support bonus');
  check(policy.InvaderAttackChance(4,10,40)===50&&policy.InvaderAttackChance(1,10,40)>50&&policy.InvaderAttackChance(8,10,40)<50,'soft target biases advances and counterattacks');
  check(policy.InvaderAttackChance(4,10,40,39,61)===61,'selection compensates for unequal industrial battle odds at equilibrium');
  check(policy.Cost(type.SUPPORT_CORRIDOR,1000)===0&&policy.Cost(type.CITY,1000)===0,'corridor/settlements excluded from simulated military capture');
  const evaluate=()=>campaign.OnEvaluation({GetRole:()=>2});
  campaignReady=false;evaluate();check(!campaign.m_bSeeded&&bank.balances[2]===10000,'no grant before campaign setup');campaignReady=true;
  evaluate();evaluate();check(campaign.m_bSeeded&&bank.balances[2]===30000&&bank.payments.length===0,'opening grant paid once with initial preparation delay');
  check(campaign.GetExternalSupportBonus()===1800,'bonus exposed for the existing income authority');
  war=5;check(campaign.GetExternalSupportBonus()===4050,'war capability can increase strategic backing while aggression remains low');
  war=2;aggression=100;check(campaign.GetExternalSupportBonus()===4500,'aggression can increase strategic backing without changing the equipment war level');aggression=10;
  const income=compile(sources.income,'GetExternalSupport');
  check(income.call({m_iOccupiersExternalSupport:500,m_iInvadersExternalSupport:500},2)===2300&&income.call({m_iOccupiersExternalSupport:500,m_iInvadersExternalSupport:500},1)===500,'only Invader external payout gains escalation bonus');
  campaign.Survey_S();check(campaign.m_aSites.length===3,'survey excludes fixed corridor');
  check(campaign.SelectTarget(2,corridor.pos,base)===near,'Invaders advance nearest the corridor first');
  near.owner=2;check(campaign.SelectTarget(2,corridor.pos,base)===far,'newly won site supports the next outward advance');
  check(campaign.SelectTarget(1,corridor.pos,base)===near,'US reclaims the exposed frontier first');near.owner=1;
  far.pos=[0,0,12000];check(campaign.SelectTarget(2,corridor.pos,base)!==far,'disconnected long-range jumps rejected');far.pos=[7802,100,4892];
  campaign.m_aPlayers=[near.pos];check(!campaign.IsDistant(near),'players block simulated battle within site plus clearance');campaign.m_aPlayers=[];
  near.active=true;check(!campaign.IsDistant(near),'physical garrisons protected');near.active=false;near.started=true;check(!campaign.IsDistant(near),'queued deployment protected');near.started=false;
  campaign.m_aOperations=[{IsTerminal:()=>false,GetTargetLocationId:()=>near.id}];check(!campaign.IsDistant(near),'active operation target protected');
  campaign.m_aOperations=[{IsTerminal:()=>false,GetTargetLocationId:()=>'',m_TacticalCommander:{m_sOriginLocationId:near.id}}];check(!campaign.IsDistant(near),'active operation origin protected');campaign.m_aOperations=[];
  pending=true;check(!campaign.IsDistant(near),'queued capture QRF protected');pending=false;
  near.owner=0;check(campaign.SelectTarget(2,corridor.pos,base)!==near,'Resistance sites excluded');const n=bank.payments.length;campaign.ResolveBattle_S(2,near);check(bank.payments.length===n&&near.owner===0,'Resistance capture cannot be overwritten');near.owner=1;
  base.owner=1;check(campaign.SelectTarget(2,corridor.pos,base)===base,'lost foothold must be re-established before expansion');base.owner=0;check(campaign.SelectTarget(2,corridor.pos,base)===0,'Resistance-held foothold blocks further expansion');base.owner=2;
  near.locked=true;check(campaign.SelectTarget(2,corridor.pos,base)!==near,'canonical flip lock honoured');near.locked=false;
  bank.balances[2]=700;reserves[2]=1;campaign.ResolveBattle_S(2,near);check(near.owner===1&&bank.balances[2]===700,'simulation cannot consume QRF collateral');reserves[2]=0;
  rolls=[0];campaign.ResolveBattle_S(2,near);check(near.owner===2&&bank.balances[2]===0,'affordable successful engagement debits attacker exactly once');
  check(bank.payments.slice(-2).map(p=>p[1]).join(',')==='700,700','both funded armies consume engagement resources');
  near.owner=1;bank.balances[2]=1000;bank.balances[1]=100;rolls=[99];campaign.ResolveBattle_S(2,near);
  check(near.owner===1&&bank.balances[2]===300&&bank.balances[1]===0,'failed attack still spends resources, depleted defender never goes negative');
  server=false;const paid=bank.payments.length;campaign.ResolveBattle_S(2,near);campaign.RestoreCampaign_S(false,false,0);check(bank.payments.length===paid&&campaign.m_bSeeded,'clients cannot fund, resolve or restore simulation');server=true;
  check(campaign.SuppressAICaptureQRF_S(1,2)&&campaign.SuppressAICaptureQRF_S(2,1)&&!campaign.SuppressAICaptureQRF_S(2,0),'AI front changes suppressed but Resistance capture still requests QRF');
  // Real capture observer must update its baseline even when skipping a simulated change.
  const observer=compile(sources.observer,'OnControlChanged'),observed={m_sId:'near',m_eRole:1};
  const adapter={m_Operations:{ReportLocationCapture_S(){reports++;}},m_aObserved:[observed]};
  near.owner=2;observer.call(adapter,near);check(reports===0&&observed.m_eRole===2,'simulated capture updates QRF observer without dispatch');
  near.owner=0;observer.call(adapter,near);check(reports===1&&observed.m_eRole===0,'later player capture uses actual displaced Invader owner');near.owner=1;
  const serialize=compile(sources.serializer,'Serialize'),deserialize=compile(sources.serializer,'Deserialize'),values={};now=1000;campaign.m_iNextBattleAt=121000;
  serialize(null,campaign,{WriteValue:(k,v)=>values[k]=v});check(values.granted&&values.delay===120,'save stores grant receipt and remaining cadence without duplicate balances');
  campaign.m_bSeeded=false;deserialize(null,campaign,{values});const balance=bank.balances[2];evaluate();check(campaign.m_bSeeded&&bank.balances[2]===balance,'load never repeats the opening grant');
  deserialize(null,campaign,{values:{version:99}});check(campaign.m_bQuarantined&&campaign.GetExternalSupportBonus()===0,'unknown state quarantines grant, funding and simulation');
  campaign.m_bQuarantined=false;campaign.m_bInvaderCampaign=false;evaluate();check(!campaign.IsAdvanceActive()&&campaign.GetExternalSupportBonus()===0&&!campaign.SuppressAICaptureQRF_S(1,2),'shared opt-out leaves existing behaviour intact');
  // Funded offscreen model on actual island sites: isolates front policy from income depletion/player interference.
  const island=catalogue.filter(r=>policy.Cost(type[r.Type],1000)>0).map(r=>makeSite(r.Id,r.Role==='INVADERS'?2:1,[+r.X,+r.Y,+r.Z],type[r.Type]));
  const foothold=island.find(s=>s.id==='everon_outpost_durras_hill');
  campaign.m_aSites=island;campaign.m_bInvaderCampaign=true;campaign.m_bQuarantined=false;campaign.m_aPlayers=[];campaign.m_aOperations=[];
  const model=goal=>{for(const s of island)s.owner=s===foothold?2:1;let sum=0,samples=0;
    for(let step=0;step<5000;step++){const held=island.filter(s=>s.owner===2).length,indI=campaign.Industry(2),indU=campaign.Industry(1),winI=policy.BattleChance(1000,1000,indI,indU),winU=policy.BattleChance(1000,1000,indU,indI);
      const attacker=random()*100<policy.InvaderAttackChance(held,island.length,goal,winI,winU)?2:1,target=campaign.SelectTarget(attacker,corridor.pos,foothold);
      if(target&&random()*100<(attacker===2?winI:winU))target.owner=attacker;
      if(step>=1000){sum+=island.filter(s=>s.owner===2).length/island.length;samples++;}}
    return sum/samples*100;};
  const baselineMean=model(40),escalatedMean=model(policy.TargetShare(40,5,100));
  check(baselineMean>34&&baselineMean<46,'funded baseline model stays near two fifths: '+baselineMean.toFixed(1));
  check(escalatedMean>baselineMean+10,'higher pressure supports a larger connected front: '+escalatedMean.toFixed(1));
  return {assertions,baselineMeanPercent:+baselineMean.toFixed(1),escalatedMeanPercent:+escalatedMean.toFixed(1),note:'Funded policy model, not native gameplay or sustained income/performance acceptance.'};
}
if(typeof module!=='undefined')module.exports={runInvaderCampaignTests};
if(typeof require!=='undefined'&&require.main===module){
  const fs=require('fs'),path=require('path'),root=path.resolve(__dirname,'../AROpenResistance/Open Resistance/Scripts/Game');
  const sources={};for(const [key,file]of Object.entries({policy:'Commanders/AROR_InvaderCampaignPolicy.c',component:'Commanders/AROR_InvaderCampaignComponent.c',serializer:'Commanders/AROR_InvaderCampaignComponentSerializer.c',observer:'Operations/AROR_QRFCaptureObserver.c',income:'Economy/AROR_IncomeComponent.c'}))sources[key]=fs.readFileSync(path.join(root,file),'utf8');
  const lines=fs.readFileSync(path.resolve(__dirname,'../docs/EVERON_CAMPAIGN_LOCATIONS.csv'),'utf8').trim().split(/\r?\n/),split=l=>l.split(',').map(x=>x.replace(/^"|"$/g,'')),header=split(lines.shift());
  const catalogue=lines.map(l=>Object.fromEntries(split(l).map((x,i)=>[header[i],x])));
  console.log(runInvaderCampaignTests(sources,catalogue));
}
