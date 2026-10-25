/* Actual editor audit orchestration with mocked native geometry/editor services.
 * No native compile, terrain or placement acceptance is implied.
 */
function runDefenceSiteAuditTests(sources) {
 let assertions=0;
 const check=(ok,message)=>{assertions++;if(!ok)throw Error(message);};
 Object.defineProperty(String.prototype,'IsEmpty',{value:function(){return this.length===0;},configurable:true});
 function body(source,name){
  const found=source.match(new RegExp('^\\t[^\\r\\n]*\\b'+name+'\\([^\\r\\n]*\\)\\r?\\n\\t\\{([\\s\\S]*?)^\\t\\}', 'm'));
  if(!found)throw Error('Missing actual method '+name);
  return found[1];
 }
 const denseSource=body(sources.layout,'DenseSiteSearchOffset')
  .replace(/\b(?:int|float) (\w+) =/g,'let $1 =')
  .replace('remaining / (ring * 2)','Math.floor(remaining / (ring * 2))');
 // Compile to a function with native integer division made explicit.
 const offset=new Function('Vector','vector','Math','return function(candidate){'+denseSource+'}')(
  (...v)=>v,{Zero:[0,0,0]},{...Math,floor:Math.floor,Mod:(a,b)=>a%b});
 const overlapSource=body(sources.layout,'FootprintsOverlap').replace(/\b(?:int|float) (\w+) =/g,'let $1 =');
 const overlap=new Function('Math','return function(center,halfExtent,yaw,otherCenter,otherHalfExtent,otherYaw,gap=8){'+overlapSource+'}')(
  {DEG2RAD:Math.PI/180,AbsFloat:Math.abs,Cos:Math.cos,Sin:Math.sin});
 let ids=Array.from({length:23},(_,i)=>i===5?'everon_outpost_andres_beacon':'everon_fixture_'+i);
 let logs=[],calls=[],failId='',missingFlag='',anchoredId='',wideSearch=false,scene='AROR_Everon_Chicken.ent';
 const sites=ids.map((id,i)=>{
  const garrison={GetDefenceHalfExtent:()=>i===0?24:18,GetMaxDefenceRelief:()=>i===0?3:2,GetEditorSearchRadius:()=>wideSearch&&i===22?1200:800,IsDefenceCenterAllowed:p=>anchoredId!==id||Math.hypot(p[0]-i*2000,p[2]-1000)<=50};
  const root={GetYawPitchRoll:()=>[0,0,0],FindComponent:()=>garrison};
  root.flag={GetOrigin:()=>[i*2000,10,1000]};
  const site={GetId:()=>id,GetOwner:()=>root,GetLocationType:()=>8};
  return {source:{GetName:()=>'AROR_Location_'+id},root,site,garrison};
 });
 const world={GetSurfaceY:()=>10};
 const api={IsEditMode:()=>true,IsPrefabEditMode:()=>false,GetWorldPath:()=>scene,
  GetEditorEntityCount:()=>ids.length,GetEditorEntity:i=>sites[i].source,
  SourceToEntity:source=>sites.find(s=>s.source===source).root,GetWorld:()=>world};
 const env={WorldEditor:{},Workbench:{GetModule:()=>({GetApi:()=>api})},
  AROR_LocationComponent:{Cast:x=>x},AROR_GarrisonComponent:{Cast:x=>x},
  AROR_CaptureLocationAction:{IsCapturableType:()=>true},
  AROR_DefenceLayout:{HalfExtent:()=>18,DenseSiteSearchOffset:offset,FootprintsOverlap:overlap},
  vector:{Zero:[0,0,0],DistanceXZ:(a,b)=>Math.hypot(a[0]-b[0],a[2]-b[2])},
  string:{Empty:'',Format:(message,...args)=>message.replace(/%(\d+)/g,(_,n)=>String(args[n-1]))},
  Print:message=>logs.push(message),LogLevel:{NORMAL:0,WARNING:1,ERROR:2},vadd:(a,b)=>a.map((v,i)=>v+b[i])};
 for(const item of sites)item.root.FindComponent=type=>type===env.AROR_LocationComponent?item.site:item.garrison;
 let runSource=body(sources.audit,'RunSiteAudit')
  .replace(/array<AROR_LocationComponent> (\w+) = \{\};/g,'let $1=[];')
  .replace(/array<(?:vector|float)> (\w+) = \{\};/g,'let $1=[];')
  .replace(/foreach \(AROR_LocationComponent (\w+) : (\w+)\)/g,'for(const $1 of $2)')
  .replace(/\b(?:WorldEditorAPI|WorldEditor|IEntitySource|IEntity|BaseWorld|AROR_LocationComponent|AROR_GarrisonComponent|vector|float|int|bool|string) (\w+);/g,'let $1=0;')
  .replace(/\b(?:WorldEditorAPI|WorldEditor|IEntitySource|IEntity|BaseWorld|AROR_LocationComponent|AROR_GarrisonComponent|vector|float|int|bool|string) (\w+) =/g,'let $1 =')
  .replace('let blocked, badArcs;','let blocked=0, badArcs=0;')
  .replace('int blocked, badArcs;','let blocked=0, badArcs=0;')
  .replace('api.GetWorldPath(path);','path=api.GetWorldPath();')
  .replace(/\.Count\(\)/g,'.length').replace(/\.Insert\(/g,'.push(')
  .replace(/\.StartsWith\(/g,'.startsWith(').replace(/\.Contains\(/g,'.includes(')
  .replace('center + offset','vadd(center,offset)');
 for(const name of ['FindFlag','CheckSite','OverlapsOtherSite'])runSource=runSource.replace(new RegExp('(?<![.\\w])'+name+'\\(', 'g'),'this.'+name+'(');
 runSource=runSource.replace('let clear = this.CheckSite(world, root, flag, site, proposed, half, proposedYaw, relief, blocked, badArcs);',
  'let measurement=this.CheckSite(world,root,flag,site,proposed,half,proposedYaw); relief=measurement.relief; blocked=measurement.blocked; badArcs=measurement.badArcs; let clear=measurement.clear;');
 const run=new Function(...Object.keys(env),'return function(targetId){'+runSource+'}')( ...Object.values(env));
 const state={FindFlag:(api,root)=>sites.find(s=>s.root===root).site.GetId()===missingFlag?null:root.flag,
  CheckSite:(world,root,flag,site,point,half,yaw)=>{calls.push({id:site.GetId(),point,half,yaw});return {clear:site.GetId()!==failId,relief:1,blocked:0,badArcs:0};},
  OverlapsOtherSite:()=>false};
 run.call(state,'');
 check(calls.length===23&&new Set(calls.map(c=>c.id)).size===23,'full audit checks every military site');
 check(logs.filter(l=>l.includes('progress=')).length===23,'full audit reports progress for all sites');
 check(calls[0].half===24&&logs.some(l=>l.includes('limit=3')),'audit uses configured footprint and terrain allowance');
 check(logs.some(l=>l.includes('begin reviewed=23 grid=25 radius=per-site'))&&logs.some(l=>l.includes('searchRadius=800')),'full dense scan declares its per-site bounds and run boundary');
 logs=[];calls=[];run.call(state,'everon_outpost_andres_beacon');
 check(calls.length===1&&calls[0].id==='everon_outpost_andres_beacon','target command still restricts itself to Andres');
 check(logs.some(l=>l.includes('reviewed=1.')),'target summary remains separately importable');
 logs=[];calls=[];
 const originalMeasure=state.CheckSite,originalSecondOrigin=sites[1].root.flag.GetOrigin;
 sites[1].root.flag.GetOrigin=()=>[1000,10,1000];
 state.CheckSite=(...args)=>{
  const result=originalMeasure(...args),id=args[3].GetId(),p=args[4];
  if(id===ids[0])result.clear=p[0]===600&&p[2]===1000;
  if(id===ids[1])result.clear=(p[0]===625||p[0]===475)&&p[2]===1000;
  return result;
 };
 run.call(state,'');
 check(logs.some(l=>l.includes('id='+ids[0]+' proposed=600,10,1000')),'first unresolved site reserves its measured proposed footprint');
 check(logs.some(l=>l.includes('id='+ids[1]+' proposed=475,10,1000')),'later site skips a conflicting first choice and selects a separated candidate');
 check(!logs.some(l=>l.includes('id='+ids[1]+' proposed=625,10,1000')),'audit cannot emit overlapping independent resource proposals');
 state.CheckSite=originalMeasure;sites[1].root.flag.GetOrigin=originalSecondOrigin;
 logs=[];calls=[];failId=ids[22];run.call(state,'');
 const failedCalls=calls.filter(c=>c.id===failId),centre=sites[22].root.flag.GetOrigin();
 check(failedCalls.length>25000,'unresolved sites receive the dense grid, not the former 129-point search');
 check(failedCalls.every(c=>Math.hypot(c.point[0]-centre[0],c.point[2]-centre[2])<=800),'every sampled orientation remains within 800m');
 check(failedCalls.some(c=>c.point[0]-centre[0]===50&&c.point[2]-centre[2]===25),'full audit samples gaps between the former eight rays');
 check(logs.some(l=>l.includes('current suitable=22 suggested relocations=0 reviewed=23.')),'failed site does not become an accepted placement');
 logs=[];calls=[];wideSearch=true;run.call(state,'');
 const wideCalls=calls.filter(c=>c.id===failId);
 check(wideCalls.some(c=>Math.hypot(c.point[0]-centre[0],c.point[2]-centre[2])>800),'configured resource search samples beyond the exhausted 800m boundary');
 check(wideCalls.every(c=>Math.hypot(c.point[0]-centre[0],c.point[2]-centre[2])<=1200),'expanded search remains bounded to 1200m');
 check(logs.some(l=>l.includes('id='+failId+' searchRadius=1200')),'expanded bounds appear in native audit evidence');wideSearch=false;
 logs=[];calls=[];anchoredId=failId;run.call(state,'');
 const anchoredCalls=calls.filter(c=>c.id===failId);
 check(anchoredCalls.length>0&&anchoredCalls.length<1000,'fixed-anchor sites do not sample distant clearing candidates');
 check(anchoredCalls.every(c=>Math.hypot(c.point[0]-centre[0],c.point[2]-centre[2])<=50),'canonical anchored capture bound applies before native geometry checks');anchoredId='';
 logs=[];calls=[];failId='';missingFlag=ids[5];run.call(state,'');
 check(logs.some(l=>l.includes('missing attached capture flag'))&&logs.some(l=>l.includes('reviewed=22.')),'missing flag cannot yield a complete 23-site result');
 logs=[];calls=[];missingFlag='';ids=ids.slice(0,22);run.call(state,'');
 check(!calls.length&&logs.some(l=>l.includes('Expected 23 military sites')),'incomplete scene refuses audit');
 ids.push('last');logs=[];calls=[];scene='Rowan.ent';run.call(state,'');
 check(!calls.length,'other scenes are never audited by Chicken command');
 return {assertions,note:'Editor orchestration and geometry services mocked; native compile and dense scan still required.'};
}
if(typeof require==='function'&&typeof module!=='undefined'&&require.main===module){
 const fs=require('fs'),path=require('path'),root=path.join(__dirname,'../AROpenResistance/Open Resistance/Scripts');
 console.log(runDefenceSiteAuditTests({audit:fs.readFileSync(path.join(root,'WorkbenchGame/Plugins/AROR_AuditDefenceSitesPlugin.c'),'utf8'),layout:fs.readFileSync(path.join(root,'Game/Garrisons/AROR_DefenceLayout.c'),'utf8')}));
}
