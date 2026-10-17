import {readdirSync,readFileSync,existsSync} from 'node:fs';
import {resolve,dirname,join} from 'node:path';
import {fileURLToPath} from 'node:url';
const root=resolve(dirname(fileURLToPath(import.meta.url)),'..');
const files=readdirSync(join(root,'scenarios')).filter(n=>n.endsWith('.json'));
for(const file of files) JSON.parse(readFileSync(join(root,'scenarios',file),'utf8'));
JSON.parse(readFileSync(join(root,'CMakePresets.json'),'utf8'));
const cmake=readFileSync(join(root,'CMakeLists.txt'),'utf8');
const paths=[...cmake.matchAll(/(?:src|apps|tests|benchmarks|ui)\/[A-Za-z0-9_./-]+\.(?:cpp|hpp|qml)/g)].map(m=>m[0]);
for(const path of paths)if(!existsSync(join(root,path)))throw new Error('Missing CMake input: '+path);
let tests=0;
function walk(path){for(const entry of readdirSync(path,{withFileTypes:true})){const file=join(path,entry.name);if(entry.isDirectory())walk(file);else if(file.endsWith('.cpp'))tests+=(readFileSync(file,'utf8').match(/TEST\(/g)||[]).length;}}
walk(join(root,'tests'));
console.log(JSON.stringify({jsonScenariosParsed:files.length,cmakeInputsPresent:new Set(paths).size,testCasesDeclared:tests,note:'This command checks assets only; it does not compile C++ or execute CTest/Qt.'},null,2));
