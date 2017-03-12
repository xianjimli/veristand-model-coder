var fs = require("fs");
var path = require("path");

function Coder() {
}

Coder.prototype.init = function(filename) {
    try {
        var str = fs.readFileSync(filename,"utf-8");
        this.json = JSON.parse(str.toString());
        return true;
    }catch(e) {
        console.dir(e);
        return false;
    }
}

Coder.prototype.gen = function(inputFileName, outputFilename, coderMapper) {
    var str = fs.readFileSync(inputFileName, "utf-8");
    for(var key in coderMapper) {
        var replaceStr = coderMapper[key]();
        str = str.replace(new RegExp(key,"gm"), replaceStr);
    }
    fs.writeFileSync(outputFilename, str);
    console.log(inputFileName + '=>' + outputFilename);
}

Coder.prototype.copyFiles = function(modelName) {
    var files = ['ni_modelframework.c', 'ni_modelframework.h'];
    files.forEach(function(filename) {
        var src = 'templates/'+filename;
        var dst = modelName+'/'+filename;

        fs.writeFileSync(dst, fs.readFileSync(src, "utf-8"));
        console.log(src +'=>' + dst);
    })
}

Coder.prototype.run = function(filename) {
    if(!this.init(filename)) {
        return;
    }
	var json = this.json;
    var name = json.name;

    if(!fs.existsSync(name)) {
        fs.mkdirSync(name);
    }

    if(json.ImplFileName) {
        this.ImplFileName = path.dirname(filename) + '/' + json.ImplFileName;
    }else{
        this.ImplFileName = 'templates/impl.c';
    }

    this.genHeader(json);
    this.genContent(json);
    this.genMakeFile(json, "CMakeLists.txt");
    this.copyFiles(name);
}

Coder.prototype.genMakeFile = function(json, filename) {
    var json = this.json;
    var name = json.name.toString();
    
    var coderMapper = {
        "@model-name@" : function() {
           return  name;
        }
    }

    this.gen("templates/"+filename, name+"/"+filename, coderMapper);
}

Coder.prototype.genHeader = function(json) {
    var json = this.json;
    var name = json.name.toString();
    var filename = name+'/model.h';
    var parameters = json.Parameters;
		
    var coderMapper = {
        "@MODEL_H@" : function() {
           return  name.toUpperCase() + '_H';
        },
        "@Parameters@" : function() {
            var str = "";
            for(var key in parameters) {
                var param = parameters[key];
                str += '\t' + param.type + ' ' + key + ';\n';
            }
            return str;
        }
    }

    this.gen("templates/model.h", filename, coderMapper);
}

Coder.prototype.toTypeMacro = function(type) {
    switch(type) {
        case 'double' : {
            return "rtDBL";
        }
        case 'int' : {
            return 'rtINT';
        }
        default: {
            return "unkown";
        }
    }
}

Coder.prototype.genContent = function() {
    var str = "";
    var coder = this;
    var json = this.json;
    var name = json.name.toString();
    var filename = name+'/'+name + '.c';
    var parameters = json.Parameters;
    var paramKeys = Object.keys(json.Parameters);
    var nparams = paramKeys.length;

    var inports = json.Inports;
    var inportKeys = Object.keys(inports);
    var ninports = inportKeys.length;

    var outports = json.Outports;
    var outportKeys = Object.keys(outports);
    var noutports = outportKeys.length;

    var signals = json.Signals;
    var signalKeys = Object.keys(signals);
    var nsignals = signalKeys.length;
    
    var coderMapper = {
        "@model-name@" : function() {
           return  name;
        },
        "@model-desc@" : function() {
           return  json.desc;
        },
        "@baserate@" : function() {
           return  json.baserate;
        },
        "@ParameterSize@" : function() {
           return nparams;
        },
        "@Inports-Decl@" : function() {
            var str = "";
            inportKeys.forEach(function(key) {
                var info = inports[key];
                str += '\t' + info.type + ' ' + key + ';\n';
            });
            return str;
        },
        "@Outports-Decl@" : function() {
            var str = "";
            outportKeys.forEach(function(key) {
                var info = outports[key];
                str += '\t' + info.type + ' ' + key + ';\n';
            });
            return str;
        },
        "@Signals-Decl@" : function() {
            var str = "";
            signalKeys.forEach(function(key) {
                var info = signals[key];
                str += '\t' + info.type + ' ' + key + ';\n';
            });
            return str;
        },
        "@rtParamAttribs@" : function() {
            var str = "";
            paramKeys.forEach(function(key, index) {
                var param = parameters[key];
                var type = coder.toTypeMacro(param.type);
                var dimListOffset = 2*index;
                var desc = param.desc || key;
                str += '\t{ 0, "' + name+'/'+desc +'", offsetof(Parameters, '+key+'), '+type+', 1, 2, '+dimListOffset+', 0}';
                str += (index+1) < paramKeys.length ? ',\n' : "";
            });
            return str;
        },
        "@ParamDimList@": function() {
            var str = "";
            paramKeys.forEach(function(key, index) {
                var param = parameters[key];
  	            str += '\t1, 1,                                /* '+name+'/'+param.desc+'*/\n';
            });
            return str;
        },
        "@initParams@": function() {
            var str = "";
            paramKeys.forEach(function(key, index) {
                var param = parameters[key];
                str += '\t'+param.value + ',/*' + key + '*/\n';
            });
            return str;
        },
        "@Parameters_sizes@": function() {
            var str = "";
            paramKeys.forEach(function(key, index) {
                var param = parameters[key];
                str += '\t{sizeof('+param.type+ '), 1, ' + coder.toTypeMacro(param.type) + '}, /*' + key + '*/\n';
            });
            return str;
        },
        "@SignalSize@": function() {
            return nsignals+ninports;
        },
        "@rtSignalAttribs@": function() {
            var str = "";
            signalKeys.forEach(function(key, index) {
                var info = signals[key];
                var dimListOffset = 2*index;
                var type = coder.toTypeMacro(info.type);
                str += '\t{ 0, "'+name+'/'+key + '", 0, "' + info.desc + '", 0, 0, ' +type+', 1, 2, '+dimListOffset+', 0},\n';
            });
            
            inportKeys.forEach(function(key, index) {
                var info = inports[key];
                var dimListOffset = 2*(index+nsignals);
                var type = coder.toTypeMacro(info.type);
                str += '\t{ 0, "'+name+'/'+key + '", 0, "' + info.desc + '", 0, 0, ' +type+', 1, 2, '+dimListOffset+', 0},\n';
            });

            return str;
        },
        "@SigDimList@": function() {
            var str = "";
            signalKeys.forEach(function(key, index) {
               str += '\t1, 1,							 /* '+name+'/'+key+' */\n'; 
            });
            
            inportKeys.forEach(function(key, index) {
               str += '\t1, 1,							 /* '+name+'/'+key+' */\n'; 
            });
            return str;
        },
        "@ExtIOSize@" : function() {
            return ninports + noutports;
        },
        "@InportSize@" : function() {
            return ninports;
        },
        "@OutportSize@" : function() {
            return noutports;
        },
        "@rtINAttribs@" : function() {
            var str = "";
            inportKeys.forEach(function(key, index) {
                str += '\t{ 0, "+'+key+'", '+index+', 0, 1, 1, 1},\n';
            });
            return str;
        },
        "@rtOutAttribs@" : function() {
            var str = "";
            outportKeys.forEach(function(key, index) {
                str += '\t{ 0, "'+key+'", '+index+', 1, 1, 1, 1},\n';
            });
            return str;
        },
        "@USER_Initialize@": function() {
            var str = "";
            signalKeys.forEach(function(key, index) {
                var info = signals[key];
                str +='\trtSignalAttribs['+index+'].addr = (uintptr_t)&rtSignal.'+key+';\n'
            });
            
            inportKeys.forEach(function(key, index) {
                var info = inports[key];
                str +='\trtSignalAttribs['+(nsignals+index)+'].addr = (uintptr_t)&rtInport.'+key+';\n'
            });

           str += "\n"; 
            signalKeys.forEach(function(key, index) {
                var info = signals[key];
                var value = info.value || "0";
                str +='\trtSignal.'+key+'='+value+';\n'
            });

            return str;
        },
        "@implementation@" : function() {
            var str = fs.readFileSync(coder.ImplFileName, "utf-8");

            return str;
        }
    }
    this.gen("templates/model.c", filename, coderMapper);
}

if(process.argv.length < 3) {
    console.log("Usage: ", process.argv[0] + " " + process.argv[1] + " model-definition.json");
    process.exit();
}

var coder = new Coder();
coder.run(process.argv[2]);
