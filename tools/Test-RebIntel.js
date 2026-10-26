/* Actual-source RebIntel regressions. Native UI/replication mocked, not an Enforce compile.
 * Run: node tools/Test-RebIntel.js
 */
function runRebIntelTests(sources) {
  let assertions=0, server=true, bumps=0, now=0, currentManager=null, currentInvasion=null;
  const check=(ok,msg)=>{assertions++;if(!ok)throw Error(msg);};
  Object.defineProperty(String.prototype,'Length',{value(){return this.length;},configurable:true});
  Object.defineProperty(Array.prototype,'Insert',{value(x){this.push(x);},configurable:true});
  const roles={RESISTANCE:0,OCCUPIERS:1,INVADERS:2};
  const apps={HOME:0,BANK:1,TEST:2,REBINTEL:3};
  const env={AROR_EFactionRole:roles,AROR_EPhoneApp:apps,Replication:{IsServer:()=>server,BumpMe:()=>bumps++},
    Math:{Floor:Math.floor,Clamp:(x,a,b)=>Math.max(a,Math.min(b,x)),Round:Math.round,Min:Math.min,Max:Math.max},
    Print(){},LogLevel:{NORMAL:0},string:{Empty:'',Format:(s,...a)=>s.replace(/%([1-9])/g,(_,n)=>a[n-1])},
    FrameSlot:{SetPos:(w,x,y)=>Object.assign(w,{x,y}),SetSize:(w,width,height)=>Object.assign(w,{width,height})},
    Color:{FromRGBA:(...rgba)=>rgba},RichTextWidget:{Cast:x=>x},ImageWidget:{Cast:x=>x},
    AROR_OperationManagerComponent:{Cast:x=>x},AROR_InvaderCampaignComponent:{Cast:x=>x},AROR_FactionRolesComponent:{GetInstance:()=>null},
    GetGame:()=>({GetWorld:()=>({GetWorldTime:()=>now}),GetGameMode:()=>({FindComponent:type=>type===env.AROR_InvaderCampaignComponent?currentInvasion:currentManager})})};
  const names=['PressureForLevel','WarLevel','GetWarPressure','GetWarLevel','HasWarState','GetAggressionPercent','IsEnemyRole','SetWarPressure_S','RestoreQRFStrategy_S',
    'IsQRFStrategyEnabled','AreInvaderOperationsEnabled','Style','Place','Label','ForceName','WarText','AggressionText','ResponseText','Init','Refresh','SetVisible','Layout',
    'OnPhoneButton','OpenApp','IsLandscapeApp','IsSettled','OnBack','RequestClose'];
  names.push('ResponseLevel','GetAggression','GetAggressionLevel','SetAggression_S');
  function compile(src,name){
    const m=src.match(new RegExp('^\\t[^\\r\\n]*\\b'+name+'\\(([^\\r\\n]*)\\)\\r?\\n\\t\\{([\\s\\S]*?)^\\t\\}', 'm'));
    if(!m)throw Error('Missing method '+name);
    const args=m[1].replace(/\b\w+\s+(\w+)/g,'$1');
    let body=m[2].replace(/\b(?:bool|int|float|string|Widget|IEntity|Faction|Color|RichTextWidget|ImageWidget|AROR_\w+)\s+(\w+)/g,'let $1')
      .replace(/let (\w+);/g,'let $1=null;').replace(/(?<![.\w])m_\w+/g,x=>'this.'+x);
    for(const n of names)body=body.replace(new RegExp('(?<![.\\w])'+n+'\\(','g'),'this.'+n+'(');
    return new Function(...Object.keys(env),'return function('+args+'){'+body+'}')( ...Object.values(env));
  }
  const policy={};env.AROR_QRFStrategicPolicy=policy;
  for(const n of ['PressureForLevel','WarLevel','ResponseLevel'])policy[n]=compile(sources.policy,n);
  const manager={m_iClock:0,m_iOccupierPressure:0,m_iInvaderPressure:0,m_iOccupierAggression:0,m_iInvaderAggression:0,m_bWarStateReady:false,m_bStrategyRestored:false,m_bQRFStrategicEconomy:true,m_bInvaderOperationsEnabled:false,
    m_QRFFinance:{SetDebt_S(){}}};
  for(const n of [...names.slice(2,11),'GetAggression','GetAggressionLevel','SetAggression_S'])manager[n]=compile(sources.manager,n);
  check(!manager.HasWarState(),'no invented readings before server initialization');
  manager.RestoreQRFStrategy_S(100,700,0,0);
  check(manager.HasWarState()&&manager.GetWarLevel(1)===2&&manager.GetWarLevel(2)===5,'restore publishes separate role readings');
  check(manager.GetAggressionPercent(1)===10&&manager.GetAggressionPercent(2)===70,'legacy restore seeds aggression once from old pressure');
  let before=bumps;manager.SetWarPressure_S(1,100);
  check(bumps===before,'unchanged pressure does not replicate again');
  manager.SetWarPressure_S(1,185);
  check(manager.GetWarLevel(1)===2&&manager.GetAggressionPercent(1)===10&&bumps===before+1,'equipment progress changes do not mutate aggression');
  manager.SetAggression_S(1,185);check(manager.GetWarLevel(1)===2&&manager.GetAggressionPercent(1)===18&&manager.GetAggressionLevel(1)===1,'aggression changes independently of equipment progress');
  check(manager.GetWarPressure(2)===700,'Occupier change leaves Invader state untouched');
  manager.SetWarPressure_S(1,-40);manager.SetWarPressure_S(2,1500);
  manager.SetAggression_S(1,-40);manager.SetAggression_S(2,1500);
  check(manager.GetAggressionPercent(1)===0&&manager.GetAggressionPercent(2)===100,'aggression bounded at both ends');
  before=bumps;manager.SetWarPressure_S(0,900);
  check(bumps===before&&manager.GetWarPressure(0)===0,'unsupported role cannot change summary');
  server=false;manager.SetWarPressure_S(1,700);manager.RestoreQRFStrategy_S(500,500,0,0);
  check(manager.GetWarPressure(1)===0&&manager.GetWarPressure(2)===1000&&bumps===before,'client cannot change or restore war pressure');
  server=true;
  const app={m_aCards:[],m_aCardBackgrounds:[],m_aNames:[],m_aRoles:[],m_aWar:[],m_aAggression:[],m_aStatus:[],m_aTracks:[],m_aBars:[],m_aPercent:[0,0],m_iNextRefresh:0};
  for(const n of names.slice(11,22))app[n]=compile(sources.app,n);
  check(app.WarText(null,1).includes('--')&&app.AggressionText(null,2).includes('--'),'unavailable intelligence uses placeholders');
  check(app.ResponseText(manager,2)==='Responses disabled','disabled Invader dispatch disclosed without changing readings');
  manager.m_bInvaderOperationsEnabled=true;manager.m_bQRFStrategicEconomy=false;
  check(app.ResponseText(manager,2)==='Strategic selection inactive','non-strategic fixture shown accurately');
  manager.m_bQRFStrategicEconomy=true;
  check(app.ResponseText(manager,1)==='Strategic selection active','active strategic selector shown');
  const bindings={GetFactionKey:r=>r===1?'AROR_OCCUPIERS':'AROR_INVADERS',GetFaction:()=>({GetFactionName:()=>'Custom Army'})};
  check(app.ForceName(bindings,1)==='US'&&app.ForceName(bindings,2)==='USSR','default forces labeled US and USSR');
  bindings.GetFactionKey=()=>'EXT_ARMY';check(app.ForceName(bindings,1)==='Custom Army','extender force name follows role mapping');
  // Parse the real layout to validate hierarchy, names and local IDs used by the app/menu.
  const widgets={},ids=[],stack=[];let lastNode=null;
  for(const line of sources.layout.split(/\r?\n/)){
    const id=line.match(/"\{([A-F0-9]{16})\}"/);if(id)ids.push(id[1]);
    if(/WidgetClass.*\{\s*$/.test(line)){lastNode={type:line.match(/(\w+WidgetClass)/)[1],parent:stack.findLast(x=>x.widget)?.widget??null};stack.push({widget:lastNode});continue;}
    if(/\{\s*$/.test(line)){stack.push({});continue;}
    const name=line.match(/^\s*Name "([^"]+)"/);if(name){const node=stack.findLast(x=>x.widget).widget;node.name=name[1];widgets[name[1]]=node;}
    if(/^\s*}\s*$/.test(line))stack.pop();
  }
  check(stack.length===0&&new Set(ids).size===ids.length,'layout balanced with unique local IDs');
  check(Object.values(widgets).every(w=>w.parent?.type!=='ImageWidgetClass'),'image widgets are leaves; frame containers own card content');
  const direct=[...sources.menu.matchAll(/FindAnyWidget\("([^"]+)"\)/g),...sources.app.matchAll(/FindAnyWidget\("([^"]+)"\)/g)].map(m=>m[1]);
  check(direct.every(n=>widgets[n]),'every direct menu/app binding resolves in the layout');
  for(const role of ['Occupiers','Invaders'])for(const suffix of ['Background','Name','Role','War','Aggression','Status','Track','Bar'])
    check(widgets['RebIntel'+role+suffix]?.parent.name==='RebIntel'+role,'role card child stays with its own faction: '+suffix);
  check(widgets.RebIntelTileButton.parent.name==='Home'&&widgets.RebIntelApp.parent.name==='AROR_PhoneMenu','tile and app belong to correct screens');
  const fakeWidgets={};
  for(const name of Object.keys(widgets))fakeWidgets[name]={name,text:'',visible:true,FindAnyWidget:n=>fakeWidgets[n],SetColor(c){this.color=c;},SetText(t){this.text=t;},GetText(){return this.text;},SetBold(){},SetExactFontSize(s){this.font=s;},SetVisible(v){this.visible=v;},SetOpacity(a){this.opacity=a;}};
  const root={FindAnyWidget:n=>fakeWidgets[n]};
  check(app.Init(root),'complete app binds successfully');
  check(!fakeWidgets.RebIntelApp.visible,'app hidden until selected');
  app.SetVisible(true,1);currentManager=manager;manager.RestoreQRFStrategy_S(100,700,0,0);
  for(const scale of [0.28,0.44,0.6]){
    app.Layout(0,0,scale);
    check(app.m_aWar[0].text.includes('2 / 5')&&app.m_aWar[1].text.includes('5 / 5'),'rendered cards retain role identity');
    check(app.m_aAggression[0].text==='Aggression   1 / 5 (10%)'&&app.m_aAggression[1].text==='Aggression   4 / 5 (70%)','displayed aggression agrees with role readings');
    check(Math.abs(app.m_aBars[0].width/(738*scale)-0.1)<1e-8&&Math.abs(app.m_aBars[1].width/(738*scale)-0.7)<1e-8,'bar lengths match percentages across scales');
    check(app.m_aCards[1].y+app.m_aCards[1].height<=fakeWidgets.RebIntelApp.height,'both cards fit inside portrait screen');
  }
  now=100;manager.SetWarPressure_S(1,450);app.Layout(0,0,0.44);
  check(app.m_aWar[0].text.includes('2 / 5'),'summary reads throttled while animating');
  now=1000;app.Layout(0,0,0.44);check(app.m_aWar[0].text.includes('4 / 5'),'live data refreshes while open');
  currentManager=null;now=2000;app.Layout(0,0,0.44);
  check(app.m_aWar[0].text.includes('--')&&!app.m_aBars[0].visible,'lost data clears stale readings and bars');
  currentManager=manager;currentInvasion={IsAdvanceActive:()=>true};now=3000;app.Layout(0,0,0.44);
  check(app.m_aStatus[1].text==='Territorial advance active'&&app.m_aStatus[0].text==='Strategic selection active','territorial campaign shown independently of physical Invader QRF admission');
  check(!sources.app.includes('AROR_OperationManagerComponent.GetInstance()'),'client does not use server-only scheduler singleton');
  check(!sources.app.includes('Rpc(')&&!sources.app.includes('SetWarPressure_S('),'app has no authority to mutate campaign state');
  const phone={m_bClosing:false,m_fTurn:0,m_fTurnTarget:0,m_eApp:apps.HOME};
  for(const n of ['OnPhoneButton','OpenApp','IsLandscapeApp','IsSettled','OnBack','RequestClose'])phone[n]=compile(sources.menu,n);
  phone.OnPhoneButton('RebIntelTileButton');check(phone.m_eApp===apps.REBINTEL&&phone.m_fTurnTarget===0,'RebIntel tile opens portrait app');
  phone.OnPhoneButton('HomeButton');check(phone.m_eApp===apps.HOME,'frame Home returns from RebIntel');
  phone.OnPhoneButton('BankTileButton');check(phone.m_eApp===apps.BANK,'Bank remains accessible');
  phone.OnPhoneButton('TestTileButton');check(phone.m_eApp===apps.TEST&&phone.m_fTurnTarget===1,'landscape app retains rotation');
  phone.OnPhoneButton('RebIntelTileButton');check(phone.m_eApp===apps.TEST,'clicks ignored while phone turning');
  phone.m_fTurn=1;phone.OnPhoneButton('HomeButton');phone.m_fTurn=0;phone.OnPhoneButton('RebIntelTileButton');
  check(phone.m_eApp===apps.REBINTEL&&phone.m_fTurnTarget===0,'return from landscape can reopen RebIntel');
  phone.OnBack();check(phone.m_bClosing,'Escape closes phone from RebIntel');
  check(/\[RplProp\(\)\]\s*protected int m_iOccupierPressure/.test(sources.manager)&&/\[RplProp\(\)\]\s*protected int m_iInvaderPressure/.test(sources.manager)&&/\[RplProp\(\)\]\s*protected bool m_bWarStateReady/.test(sources.manager),'summary fields included in engine replication');
  check(/\[RplProp\(\)\]\s*protected int m_iOccupierAggression/.test(sources.manager)&&/\[RplProp\(\)\]\s*protected int m_iInvaderAggression/.test(sources.manager),'independent aggression is included in engine replication');
  return {assertions,note:'Native rendering, replication/JIP and Enforce compilation still require Workbench.'};
}
if(typeof module!=='undefined')module.exports={runRebIntelTests};
if(typeof require!=='undefined'&&require.main===module){
  const fs=require('fs'),path=require('path'),root=path.resolve(__dirname,'../AROpenResistance/Open Resistance');
  const read=p=>fs.readFileSync(path.join(root,p),'utf8');
  console.log(runRebIntelTests({policy:read('Scripts/Game/Operations/AROR_QRFStrategicPolicy.c'),manager:read('Scripts/Game/Operations/AROR_OperationManagerComponent.c'),
    app:read('Scripts/Game/Phone/AROR_RebIntelApp.c'),menu:read('Scripts/Game/Phone/AROR_PhoneMenu.c'),layout:read('UI/Layouts/Phone/AROR_PhoneMenu.layout')}));
}
