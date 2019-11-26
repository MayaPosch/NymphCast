
#include "jsonfile.h"
#include "jsonvalue.h"

void initJson(asIScriptEngine* engine) {
	RegisterJSONValue(engine);
	RegisterJSONFile(engine);
}
