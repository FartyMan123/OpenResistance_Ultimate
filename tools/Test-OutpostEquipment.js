/* Actual supply audit methods with mocked engine inventories. No native loot/UI acceptance. */
function runOutpostEquipmentTests(source) {
 let assertions=0,server=true,play=true,owner=null,logs=[],timers=[],removed=[];
 const check=(value,message)=>{assertions++;if(!value)throw Error(message);};
 for(const [name,fn]of Object.entries({IsEmpty(){return this.length===0;},Count(){return this.length;}}))Object.defineProperty(Array.prototype,name,{value:fn,configurable:true});
 const env={Replication:{IsServer:()=>server},AROR_LocationComponent:{Cast:x=>x},InventoryStorageManagerComponent:{Cast:x=>x},
  Print:(message,level)=>logs.push({message,level}),LogLevel:{NORMAL:0,WARNING:1,ERROR:2},
  string:{Format:(message,...values)=>message.replace(/%(\d+)/g,(_,i)=>String(values[i-1]))},
  GetGame:()=>({InPlayMode:()=>play,GetCallqueue:()=>({CallLater:(...args)=>timers.push(args),Remove:method=>removed.push(method)})})};
 function compile(name){
  const match=source.match(new RegExp('^\\t[^\\r\\n]*\\b'+name+'\\([^\\r\\n]*\\)\\r?\\n\\t\\{([\\s\\S]*?)^\\t\\}', 'm'));
  if(!match)throw Error('Missing actual supply method '+name);
  const body=match[1].replace(/super\.\w+\([^;]*\);/g,'')
   .replace('array<IEntity> items = {};','let items=[];')
   .replace(/\b(?:int|string|IEntity|AROR_LocationComponent|InventoryStorageManagerComponent) (\w+) =/g,'let $1 =')
   .replace(/\bGetOwner\(/g,'this.GetOwner(').replace(/\bm_sCategory\b/g,'this.m_sCategory')
   .replace(/\bCheckContents_S\b/g,'this.CheckContents_S');
  return new Function(...Object.keys(env),'return function('+(name==='CheckContents_S'?'':'owner')+'){'+body+'}')( ...Object.values(env));
 }
 const inspect=compile('CheckContents_S'),init=compile('OnPostInit'),dispose=compile('OnDelete');
 const component={GetOwner:()=>owner,m_sCategory:'Weapons',CheckContents_S:inspect};
 let reads=0;
 const inventory={GetItems:items=>{reads++;items.push({weapon:true},{magazine:true});}};
 const parent={FindComponent:()=>({GetId:()=> 'everon_test_outpost'})};
 owner={GetParent:()=>parent,FindComponent:()=>inventory};
 inspect.call(component);
 check(reads===1&&logs.some(l=>l.message.includes('observedItems=2')),'audit reports physical item observations');
 check(logs[0].message.includes('site=everon_test_outpost')&&logs[0].message.includes('category=Weapons'),'inventory results identify the site and category');
 logs=[];owner.FindComponent=()=>({GetItems:()=>0});inspect.call(component);
 check(logs.some(l=>l.level===1&&l.message.includes('observedItems=0')),'empty or previously looted crates are reported');
 check(logs.every(l=>l.message.includes('no refill performed')),'empty inventory never triggers a restock');
 logs=[];owner.FindComponent=()=>null;inspect.call(component);
 check(logs.some(l=>l.level===2&&l.message.includes('missing native inventory manager')),'incompatible container assets fail visibly');
 logs=[];owner=null;inspect.call(component);check(!logs.length,'deleted containers are safe');
 owner={GetParent:()=>parent,FindComponent:()=>inventory};server=false;inspect.call(component);
 check(!logs.length&&reads===1,'clients never inspect or seed server stock');
 init.call(component,owner);check(!timers.length,'client initialization schedules no inventory work');
 server=true;play=false;init.call(component,owner);check(!timers.length,'edit mode does not inspect inventories');
 play=true;init.call(component,owner);
 check(timers.length===1&&timers[0][1]===5000&&timers[0][2]===false,'server schedules one delayed inspection, not a refill loop');
 dispose.call(component,owner);check(removed.length===1&&removed[0]===inspect,'deletion removes the pending inspection');
 let parentQueries=0;const cycle={FindComponent:()=>{parentQueries++;return null;},GetParent:()=>cycle};
 logs=[];owner={GetParent:()=>cycle,FindComponent:()=>inventory};inspect.call(component);
 check(parentQueries===16&&logs[0].message.includes('site=unregistered'),'unexpected parent hierarchies are bounded');
 return {assertions,note:'Native inventories mocked. Container contents, access, depletion and save/load require Workbench.'};
}
if(typeof require==='function'&&typeof module!=='undefined'&&require.main===module){
 const fs=require('fs'),path=require('path');
 console.log(runOutpostEquipmentTests(fs.readFileSync(path.join(__dirname,'../AROpenResistance/Open Resistance/Scripts/Game/Garrisons/AROR_OutpostSupplyComponent.c'),'utf8')));
}
