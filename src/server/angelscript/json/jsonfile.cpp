//
// Copyright (c) 2008-2019 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.
//

/* #include "../Precompiled.h"

#include "../Container/ArrayPtr.h"
#include "../Core/Profiler.h"
#include "../Core/Context.h"
#include "../IO/Deserializer.h"
#include "../IO/Log.h"
#include "../IO/MemoryBuffer.h"
#include "../Resource/JSONFile.h"
#include "../Resource/ResourceCache.h" */

/* #include <rapidjson/document.h>
#include <rapidjson/stringbuffer.h>
#include <rapidjson/prettywriter.h> */


//#include "../DebugNew.h"


#include "jsonfile.h"


static void ConstructJSONFile(JSONFile* ptr) {
    new(ptr) JSONFile();
}

static void DestructJSONFile(JSONFile* ptr) {
    ptr->~JSONFile();
}


void RegisterJSONFile(asIScriptEngine* engine) {
	/* std::string declFactory("JSONFile@ f()");
	engine->RegisterObjectBehaviour(className, asBEHAVE_FACTORY, declFactory.CString(), asFUNCTION(ConstructObject<T>), asCALL_CDECL);
	
	std::string declFactoryWithName(std::string("JSONFile@ f(const string&in)");
	engine->RegisterObjectBehaviour(className, asBEHAVE_FACTORY, declFactoryWithName.CString(), asFUNCTION(ConstructNamedObject<T>), asCALL_CDECL); */
	
	//engine->RegisterObjectType("JSONFile", sizeof(JSONFile), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
	engine->RegisterObjectType("JSONFile", sizeof(JSONFile), asOBJ_VALUE);
    engine->RegisterObjectMethod("JSONFile", "bool fromString(const string&in)", asMETHOD(JSONFile, fromString), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONFile", "string toString(const string&in = string(\"\t\")) const", asMETHOD(JSONFile, toString), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONFile", "JSONValue& getRoot()", asMETHODPR(JSONFile, getRoot, () const, const JSONValue&), asCALL_THISCALL);
    //engine->RegisterObjectMethod("JSONFile", "bool Save(File@+, const string&in) const", asFUNCTION(JSONFileSave), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("JSONFile", "JSONValue& get_root()", asMETHODPR(JSONFile, getRoot, () const, const JSONValue&), asCALL_THISCALL);
	
	engine->RegisterObjectBehaviour("JSONFile", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructJSONFile), asCALL_CDECL_OBJLAST);
	engine->RegisterObjectBehaviour("JSONFile", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructJSONFile), asCALL_CDECL_OBJLAST);
}


JSONFile::JSONFile(/* Context* context */)// :
    //Resource(context)
{
}

JSONFile::~JSONFile() = default;

//void JSONFile::RegisterObject(/* Context* context */) {
    //context->RegisterFactory<JSONFile>();
//}


// Convert Poco JSON value to JSON value.
/* static void toJSONValue(JSONValue& jsonValue, const Poco::Dynamic::Value& pocoValue) {
    switch (pocoValue.GetType()) {
    case kNullType:
        // Reset to null type
        jsonValue.SetType(JSON_NULL);
        break;

    case kFalseType:
        jsonValue = false;
        break;

    case kTrueType:
        jsonValue = true;
        break;

    case kNumberType:
        if (rapidjsonValue.IsInt())
            jsonValue = rapidjsonValue.GetInt();
        else if (rapidjsonValue.IsUint())
            jsonValue = rapidjsonValue.GetUint();
        else
            jsonValue = rapidjsonValue.GetDouble();
        break;

    case kStringType:
        jsonValue = rapidjsonValue.GetString();
        break;

    case kArrayType:
        {
            jsonValue.Resize(rapidjsonValue.Size());
            for (unsigned i = 0; i < rapidjsonValue.Size(); ++i)
            {
                ToJSONValue(jsonValue[i], rapidjsonValue[i]);
            }
        }
        break;

    case kObjectType:
        {
            jsonValue.SetType(JSON_OBJECT);
            for (rapidjson::Value::ConstMemberIterator i = rapidjsonValue.MemberBegin(); i != rapidjsonValue.MemberEnd(); ++i)
            {
                JSONValue& value = jsonValue[String(i->name.GetString())];
                ToJSONValue(value, i->value);
            }
        }
        break;

    default:
        break;
    }
} */


/* bool JSONFile::BeginLoad(Deserializer& source) {
    unsigned dataSize = source.GetSize();
    if (!dataSize && !source.GetName().Empty())
    {
        URHO3D_LOGERROR("Zero sized JSON data in " + source.GetName());
        return false;
    }

    SharedArrayPtr<char> buffer(new char[dataSize + 1]);
    if (source.Read(buffer.Get(), dataSize) != dataSize)
        return false;
    buffer[dataSize] = '\0';

    rapidjson::Document document;
    if (document.Parse<kParseCommentsFlag | kParseTrailingCommasFlag>(buffer).HasParseError())
    {
        URHO3D_LOGERROR("Could not parse JSON data from " + source.GetName());
        return false;
    }

    ToJSONValue(root_, document);

    SetMemoryUse(dataSize);

    return true;
} */


/* static void ToRapidjsonValue(rapidjson::Value& rapidjsonValue, const JSONValue& jsonValue, rapidjson::MemoryPoolAllocator<>& allocator)
{
    switch (jsonValue.GetValueType())
    {
    case JSON_NULL:
        rapidjsonValue.SetNull();
        break;

    case JSON_BOOL:
        rapidjsonValue.SetBool(jsonValue.GetBool());
        break;

    case JSON_NUMBER:
        {
            switch (jsonValue.GetNumberType())
            {
            case JSONNT_INT:
                rapidjsonValue.SetInt(jsonValue.GetInt());
                break;

            case JSONNT_UINT:
                rapidjsonValue.SetUint(jsonValue.GetUInt());
                break;

            default:
                rapidjsonValue.SetDouble(jsonValue.GetDouble());
                break;
            }
        }
        break;

    case JSON_STRING:
        rapidjsonValue.SetString(jsonValue.GetCString(), allocator);
        break;

    case JSON_ARRAY:
        {
            const JSONArray& jsonArray = jsonValue.GetArray();

            rapidjsonValue.SetArray();
            rapidjsonValue.Reserve(jsonArray.Size(), allocator);

            for (unsigned i = 0; i < jsonArray.Size(); ++i) {
                rapidjson::Value value;
                ToRapidjsonValue(value, jsonArray[i], allocator);
                rapidjsonValue.PushBack(value, allocator);
            }
        }
        break;

    case JSON_OBJECT:
        {
            const JSONObject& jsonObject = jsonValue.GetObject();

            rapidjsonValue.SetObject();
            for (JSONObject::ConstIterator i = jsonObject.Begin(); i != jsonObject.End(); ++i) {
                const char* name = i->first_.CString();
                rapidjson::Value value;
                ToRapidjsonValue(value, i->second_, allocator);
                rapidjsonValue.AddMember(StringRef(name), value, allocator);
            }
        }
        break;

    default:
        break;
    }
} */


/* bool JSONFile::Save(Serializer& dest) const {
    return Save(dest, "\t");
}


bool JSONFile::Save(Serializer& dest, const std::string& indendation) const {
    rapidjson::Document document;
    ToRapidjsonValue(document, root_, document.GetAllocator());

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    writer.SetIndent(!indendation.Empty() ? indendation.Front() : '\0', indendation.Length());

    document.Accept(writer);
    auto size = (unsigned)buffer.GetSize();
    return dest.Write(buffer.GetString(), size) == size;
	
	return 
} */


bool JSONFile::fromString(const std::string &json) {
    if (json.empty()) { return false; }
	
	Poco::JSON::Parser parser;
	//Poco::Dynamic::Var result = parser.parse(json);
	Poco::Dynamic::Var result = parser.parse(json);
	
	root_.setVariant(result);
	
	/* if (result.type() == typeid(Poco::JSON::Object::Ptr)) {
		Poco::JSON::Object::Ptr object = result.extract<Poco::JSON::Object::Ptr>();
		root_.SetType(JSON_OBJECT);
		root_ = object;
	}
	else if (result.type() == typeid(Poco::JSON::Array::Ptr)) {
		Poco::JSON::Array::Ptr object = result.extract<Poco::JSON::Array::Ptr>();
		
		root_.SetType(JSON_ARRAY);
		root_ = object;
	}
	else {
		return false;
	} */

    /* MemoryBuffer buffer(source.CString(), source.Length());
    return Load(buffer); */
	
	return true;
}


std::string JSONFile::toString(const std::string& indentation) const {
    /* rapidjson::Document document;
    ToRapidjsonValue(document, root_, document.GetAllocator());

    StringBuffer buffer;
    PrettyWriter<StringBuffer> writer(buffer);
    writer.SetIndent(!indendation.Empty() ? indendation.Front() : '\0', indendation.Length());

    document.Accept(writer);
    return buffer.GetString(); */
	
	return std::string();
}
