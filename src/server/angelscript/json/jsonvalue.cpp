//
// Copyright (c) 2008-2019 the Urho3D project.
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:addmember
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

#include "../Core/Context.h"
#include "../Core/StringUtils.h"
#include "../IO/Log.h"
#include "../Resource/JSONValue.h"

#include "../DebugNew.h" */


#include "jsonvalue.h"


#include <iostream>


static void ConstructJSONValue(JSONValue* ptr) {
    new(ptr) JSONValue();
}

static void ConstructJSONValueBool(bool value, JSONValue* ptr) {
    new (ptr) JSONValue(value);
}

static void ConstructJSONValueInt(int value, JSONValue* ptr) {
    new (ptr) JSONValue(value);
}

static void ConstructJSONValueUnsigned(unsigned value, JSONValue* ptr) {
    new (ptr) JSONValue(value);
}

static void ConstructJSONValueFloat(float value, JSONValue* ptr) {
    new (ptr) JSONValue(value);
}

static void ConstructJSONValueDouble(double value, JSONValue* ptr) {
    new (ptr) JSONValue(value);
}

static void ConstructJSONValueString(const std::string& value, JSONValue* ptr) {
    new (ptr) JSONValue(value);
}

static void ConstructJSONValueCopy(const JSONValue& value, JSONValue* ptr) {
    new(ptr) JSONValue(value);
}

static void DestructJSONValue(JSONValue* ptr) {
    ptr->~JSONValue();
}

static JSONValue JSONValueAtPosition(unsigned position, JSONValue& jsonValue) {
    return jsonValue[position];
}

static JSONValue& JSONValueAtKey(const std::string& key, JSONValue& jsonValue) {
    return jsonValue[key];
}

/* static CScriptArray* JSONValueGetKeys(const JSONValue& jsonValue) {
    Vector<std::string> keys;
    if (jsonValue.IsObject())
    {
        for (ConstJSONObjectIterator i = jsonValue.Begin(); i != jsonValue.End(); ++i)
            keys.Push(i->first_);
    }

    return VectorToArray<std::string>(keys, "Array<String>");
} */


void RegisterJSONValue(asIScriptEngine* engine) {
    engine->RegisterEnum("JSONValueType");
    engine->RegisterEnumValue("JSONValueType", "JSON_NULL", JSON_NULL);
    engine->RegisterEnumValue("JSONValueType", "JSON_BOOL", JSON_BOOL);
    engine->RegisterEnumValue("JSONValueType", "JSON_NUMBER", JSON_NUMBER);
    engine->RegisterEnumValue("JSONValueType", "JSON_STRING", JSON_STRING);
    engine->RegisterEnumValue("JSONValueType", "JSON_ARRAY", JSON_ARRAY);
    engine->RegisterEnumValue("JSONValueType", "JSON_OBJECT", JSON_OBJECT);

    engine->RegisterEnum("JSONNumberType");
    engine->RegisterEnumValue("JSONNumberType", "JSONNT_NAN", JSONNT_NAN);
    engine->RegisterEnumValue("JSONNumberType", "JSONNT_INT", JSONNT_INT);
    engine->RegisterEnumValue("JSONNumberType", "JSONNT_UINT", JSONNT_UINT);
    engine->RegisterEnumValue("JSONNumberType", "JSONNT_FLOAT_DOUBLE", JSONNT_FLOAT_DOUBLE);

    engine->RegisterObjectType("JSONValue", sizeof(JSONValue), asOBJ_VALUE | asOBJ_APP_CLASS_CDAK);
    engine->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void f()", asFUNCTION(ConstructJSONValue), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void f(bool)", asFUNCTION(ConstructJSONValueBool), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void f(int)", asFUNCTION(ConstructJSONValueInt), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void f(uint)", asFUNCTION(ConstructJSONValueUnsigned), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void f(float)", asFUNCTION(ConstructJSONValueFloat), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void f(double)", asFUNCTION(ConstructJSONValueDouble), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void f(const string&in)", asFUNCTION(ConstructJSONValueString), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour("JSONValue", asBEHAVE_CONSTRUCT, "void f(const JSONValue&in)", asFUNCTION(ConstructJSONValueCopy), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectBehaviour("JSONValue", asBEHAVE_DESTRUCT, "void f()", asFUNCTION(DestructJSONValue), asCALL_CDECL_OBJLAST);

    engine->RegisterObjectMethod("JSONValue", "JSONValue& opAssign(bool)", asMETHODPR(JSONValue, operator =, (bool), JSONValue&), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "JSONValue& opAssign(int)", asMETHODPR(JSONValue, operator =, (int), JSONValue&), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "JSONValue& opAssign(uint)", asMETHODPR(JSONValue, operator =, (unsigned), JSONValue&), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "JSONValue& opAssign(float)", asMETHODPR(JSONValue, operator =, (float), JSONValue&), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "JSONValue& opAssign(double)", asMETHODPR(JSONValue, operator =, (double), JSONValue&), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "JSONValue& opAssign(const string&in)", asMETHODPR(JSONValue, operator =, (const std::string&), JSONValue&), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "JSONValue& opAssign(const JSONValue&in)", asMETHODPR(JSONValue, operator =, (const JSONValue&), JSONValue&), asCALL_THISCALL);
    /* engine->RegisterObjectMethod("JSONValue", "void SetVariant(const Variant&in)", asFUNCTION(JSONValueSetVariant), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("JSONValue", "void SetVariantMap(const VariantMap&in)", asFUNCTION(JSONValueSetVariantMap), asCALL_CDECL_OBJLAST); */

    engine->RegisterObjectMethod("JSONValue", "JSONValueType get_valueType() const", asMETHOD(JSONValue, GetValueType), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "JSONNumberType get_numberType() const", asMETHOD(JSONValue, GetNumberType), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "string get_valueTypeName() const", asMETHODPR(JSONValue, GetValueTypeName, () const, std::string), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "string get_numberTypeName() const", asMETHODPR(JSONValue, GetNumberTypeName, () const, std::string), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "bool get_isNull() const", asMETHOD(JSONValue, IsNull), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "bool get_isBool() const", asMETHOD(JSONValue, IsBool), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "bool get_isNumber() const", asMETHOD(JSONValue, IsNumber), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "bool get_isString() const", asMETHOD(JSONValue, IsString), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "bool get_isArray() const", asMETHOD(JSONValue, IsArray), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "bool get_isObject() const", asMETHOD(JSONValue, IsObject), asCALL_THISCALL);

    engine->RegisterObjectMethod("JSONValue", "bool getBool(bool defaultValue = false) const", asMETHOD(JSONValue, GetBool), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "int getInt(int defaultValue = 0) const", asMETHOD(JSONValue, GetInt), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "uint getUInt(uint defaultValue = 0) const", asMETHOD(JSONValue, GetUInt), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "float getFloat(float defaultValue = 0) const", asMETHOD(JSONValue, GetFloat), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "double getDouble(double defaultValue = 0) const", asMETHOD(JSONValue, GetDouble), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "const string getString(const string&in defaultValue = string()) const", asMETHOD(JSONValue, GetString), asCALL_THISCALL);
    //engine->RegisterObjectMethod("JSONValue", "Variant GetVariant(Variant defaultValue = Variant()) const", asMETHOD(JSONValue, GetVariant), asCALL_THISCALL);
    //engine->RegisterObjectMethod("JSONValue", "VariantMap GetVariantMap(VariantMap defaultValue = VariantMap()) const", asMETHOD(JSONValue, GetVariantMap), asCALL_THISCALL);

    engine->RegisterObjectMethod("JSONValue", "JSONValue opIndex(uint)", asFUNCTION(JSONValueAtPosition), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("JSONValue", "const JSONValue& opIndex(uint) const", asFUNCTION(JSONValueAtPosition), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("JSONValue", "void Push(const JSONValue&in)", asMETHOD(JSONValue, Push), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "void Pop()", asMETHOD(JSONValue, Pop), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "void Insert(uint, const JSONValue&in)", asMETHODPR(JSONValue, Insert, (unsigned, const JSONValue&), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "void Erase(uint, uint length = 1)", asMETHODPR(JSONValue, Erase, (unsigned, unsigned), void), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "void Resize(uint)", asMETHOD(JSONValue, Resize), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "uint get_size() const", asMETHOD(JSONValue, size), asCALL_THISCALL);

    engine->RegisterObjectMethod("JSONValue", "JSONValue& opIndex(const string&in)", asFUNCTION(JSONValueAtKey), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("JSONValue", "const JSONValue& opIndex(const string&in) const", asFUNCTION(JSONValueAtKey), asCALL_CDECL_OBJLAST);
    engine->RegisterObjectMethod("JSONValue", "void Set(const string&in, const JSONValue&in)", asMETHOD(JSONValue, Set), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "JSONValue get(string&in)", asMETHOD(JSONValue, get), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "void Erase(const string&in)", asMETHODPR(JSONValue, Erase, (const std::string&), bool), asCALL_THISCALL);
    engine->RegisterObjectMethod("JSONValue", "bool Contains(const string&in) const", asMETHOD(JSONValue, Contains), asCALL_THISCALL);

    engine->RegisterObjectMethod("JSONValue", "void Clear()", asMETHOD(JSONValue, Clear), asCALL_THISCALL);

    //engine->RegisterObjectMethod("JSONValue", "Array<String>@ get_keys() const", asFUNCTION(JSONValueGetKeys), asCALL_CDECL_OBJLAST);
    //engine->RegisterObjectMethod("JSONValue", "Array<JSONValue>@ get_values() const", asFUNCTION(JSONValueGetValues), asCALL_CDECL_OBJLAST);
}


static const char* valueTypeNames[] =
{
    "Null",
    "Bool",
    "Number",
    "String",
    "Array",
    "Object",
    nullptr
};

static const char* numberTypeNames[] =
{
    "NaN",
    "Int",
    "Unsigned",
    "Real",
    nullptr
};

const JSONValue JSONValue::EMPTY;
JSONValue JSONValue::emptyValue;
const JSONArray JSONValue::emptyArray { };
const JSONObject JSONValue::emptyObject;

JSONValue& JSONValue::operator =(bool rhs) {
    SetType(JSON_BOOL);
    boolValue_ = rhs;

    return *this;
}

JSONValue& JSONValue::operator =(int rhs) {
    //SetType(JSON_NUMBER, JSONNT_INT);
    SetType(JSON_NUMBER);
    numberValue_ = rhs;
	
	std::cout << "New JSON int: " << rhs << std::endl;

    return *this;
}

JSONValue& JSONValue::operator =(unsigned rhs) {
    //SetType(JSON_NUMBER, JSONNT_UINT);
    SetType(JSON_NUMBER);
    numberValue_ = rhs;
	
	std::cout << "New JSON uint: " << rhs << std::endl;

    return *this;
}

JSONValue& JSONValue::operator =(float rhs) {
    //SetType(JSON_NUMBER, JSONNT_FLOAT_DOUBLE);
    SetType(JSON_NUMBER);
    numberValue_ = rhs;

    return *this;
}

JSONValue& JSONValue::operator =(double rhs) {
    //SetType(JSON_NUMBER, JSONNT_FLOAT_DOUBLE);
    SetType(JSON_NUMBER);
    numberValue_ = rhs;

    return *this;
}

JSONValue& JSONValue::operator =(const std::string& rhs) {
    SetType(JSON_STRING);
    *stringValue_ = rhs;
	
	std::cout << "New JSONString: " << rhs << std::endl;

    return *this;
}

JSONValue& JSONValue::operator =(const char* rhs) {
    SetType(JSON_STRING);
    *stringValue_ = rhs;

    return *this;
}

JSONValue& JSONValue::operator =(const JSONArray& rhs) {
    SetType(JSON_ARRAY);
    *arrayValue_ = rhs;
	
	std::cout << "New JSON array." << std::endl;

    return *this;
}

JSONValue& JSONValue::operator =(const JSONObject& rhs) {
    SetType(JSON_OBJECT);
    *objectValue_ = rhs;
	
	std::cout << "New JSON object." << std::endl;

    return *this;
}

JSONValue& JSONValue::operator =(const JSONValue& rhs) {
    if (this == &rhs) { return *this; }
	
	value = rhs.value;
	isArrayType = rhs.isArrayType;
	isObjectType = rhs.isObjectType;
	objectPtr = rhs.objectPtr;
	arrayPtr = rhs.arrayPtr;

    //SetType(rhs.GetValueType(), rhs.GetNumberType());
    //SetType(rhs.GetValueType());

    /* switch (GetValueType()) {
    case JSON_BOOL:
        boolValue_ = rhs.boolValue_;
        break;

    case JSON_NUMBER:
        numberValue_ = rhs.numberValue_;
        break;

    case JSON_STRING:
        *stringValue_ = *rhs.stringValue_;
        break;

    case JSON_ARRAY:
        *arrayValue_ = *rhs.arrayValue_;
        break;

    case JSON_OBJECT:
        *objectValue_ = *rhs.objectValue_;

    default:
        break;
    } */

    return *this;
}

JSONValueType JSONValue::GetValueType() const {
    //return (JSONValueType)(type_ >> 16u);
	return (JSONValueType) type_;
}

JSONNumberType JSONValue::GetNumberType() const {
    //return (JSONNumberType)(type_ & 0xffffu);
	return (JSONNumberType) type_;
}

std::string JSONValue::GetValueTypeName() const {
    return GetValueTypeName(GetValueType());
}

std::string JSONValue::GetNumberTypeName() const {
    return GetNumberTypeName(GetNumberType());
}

JSONValue JSONValue::operator [](unsigned index) {
	if (!isArrayType) { return emptyValue; }
	
	std::cout << "Retrieving array index " << index << std::endl;

    //return (*arrayValue_)[index];
	// TODO: implement
	Poco::Dynamic::Var value = arrayPtr->get(index);
	if (value.isEmpty()) { 
		std::cerr << "Failed to find index." << std::endl;
		return emptyValue; 
	}
	
	JSONValue val;
	val.setVariant(value);
	return val;
	/* if (value.isNull()) {
		val.SetType(JSON_NULL);
	}
	else *//*  if (value.isBoolean()) {
		//val.SetType(JSON_BOOL);
		val = value.convert<bool>();
	}
	else if (value.isNumeric()) {
		//val.SetType(JSON_NUMBER);
		val = value.convert<double>();
		
		std::cout << "Found double." << std::endl;
	}
	else if (value.isString()) {
		//val.SetType(JSON_STRING);
		val = value.convert<std::string>();
		
		std::cout << "Found string." << std::endl;
	}
	else if (value.type() == typeid(Poco::JSON::Object::Ptr)) {
		val.SetType(JSON_OBJECT);
		val = (JSONObject) value.extract<Poco::JSON::Object::Ptr>();
		
		std::cout << "Found object." << std::endl;
	}
	else if (value.type() == typeid(Poco::JSON::Array::Ptr)) {
		val.SetType(JSON_ARRAY);
		val = (JSONArray) value.extract<Poco::JSON::Array::Ptr>();
		
		std::cout << "Found double." << std::endl;
	}
	else {
		return EMPTY;
	} */
	
	return emptyValue;
}

const JSONValue& JSONValue::operator [](unsigned index) const {
    if (!isArrayType) { return emptyValue; }
	
	std::cout << "Retrieving const array index " << index << std::endl;

    //return (*arrayValue_)[index];
	// TODO: implement
	Poco::Dynamic::Var value = arrayPtr->get(index);
	if (value.isEmpty()) { return emptyValue; }
	
	JSONValue val;
	val.setVariant(value);
	return val;
	
	return emptyValue;
}

void JSONValue::Push(const JSONValue& value) {
    // Convert to array type
    SetType(JSON_ARRAY);

    //arrayValue_->Push(value);
}

void JSONValue::Pop() {
    if (GetValueType() != JSON_ARRAY)
        return;

    //arrayValue_->Pop();
}

void JSONValue::Insert(unsigned pos, const JSONValue& value) {
    if (GetValueType() != JSON_ARRAY)
        return;

    //arrayValue_->Insert(pos, value);
}

void JSONValue::Erase(unsigned pos, unsigned length) {
    if (GetValueType() != JSON_ARRAY)
        return;

    //arrayValue_->Erase(pos, length);
}

void JSONValue::Resize(unsigned newSize) {
    // Convert to array type
    SetType(JSON_ARRAY);

    //arrayValue_->Resize(newSize);
}

unsigned JSONValue::size() const {
	if (isArrayType) { 
		std::cout << "Array size: " << arrayPtr->size() << std::endl;
		return arrayPtr->size(); 
	}
	if (isObjectType) { 
		std::cout << "Object size: " << objectPtr->size() << std::endl;
		return objectPtr->size(); 
	}
	
    /* if (GetValueType() == JSON_ARRAY) { return (*arrayValue_)->size(); }
    else if (GetValueType() == JSON_OBJECT) { return (*objectValue_)->size(); } */

    return 0;
}

JSONValue& JSONValue::operator [](const std::string& key) {
    // Convert to object type
    SetType(JSON_OBJECT);

    //return (*objectValue_)[key];
	return emptyValue;
}

const JSONValue& JSONValue::operator [](const std::string& key) const {
    if (GetValueType() != JSON_OBJECT)
        return emptyValue;

    //return (*objectValue_)[key];
	return emptyValue;
}

void JSONValue::Set(const std::string& key, const JSONValue& value) {
    // Convert to object type
    SetType(JSON_OBJECT);

    //(*objectValue_)[key] = value;
}

JSONValue JSONValue::get(std::string& key) {
	std::cout << "Checking for object... " << isObjectType << "/" << isArrayType << std::endl;
	std::cout << "Searching for key: " << key << std::endl;
	
    //if (GetValueType() != JSON_OBJECT) { return EMPTY; }
	if (!isObjectType) { return EMPTY; }
	
	std::cout << "Retrieving value from object. Key: " << key << std::endl;

    Poco::Dynamic::Var value = objectPtr->get(key);
	if (value.isEmpty()) {
		std::cerr << "Key wasn't found." << std::endl;
		return EMPTY; 
	}
	
	JSONValue val;
	val.setVariant(value);
	
	/* if (value.isNull()) {
		val.SetType(JSON_NULL);
	}
	else */ /* if (value.isBoolean()) {
		val.SetType(JSON_BOOL);
		val = value.convert<bool>();
	}
	else if (value.isNumeric()) {
		val.SetType(JSON_NUMBER);
		val = value.convert<double>();
		
		std::cout << "Retrieved double." << std::endl;
	}
	else if (value.isString()) {
		val.SetType(JSON_STRING);
		val = value.convert<std::string>();
		
		std::cout << "Retrieved string." << std::endl;
	}
	else if (value.type() == typeid(Poco::JSON::Object::Ptr)) {
		val.SetType(JSON_OBJECT);
		val = (JSONObject) value.extract<Poco::JSON::Object::Ptr>();
		
		std::cout << "Retrieved object." << std::endl;
	}
	else if (value.type() == typeid(Poco::JSON::Array::Ptr)) {
		val.SetType(JSON_ARRAY);
		val = (JSONArray) value.extract<Poco::JSON::Array::Ptr>();
		
		std::cout << "Retrieved array." << std::endl;
	}
	else {
		std::cout << "Default: empty value." << std::endl;
		return EMPTY;*/
	//}

    return val;
}

bool JSONValue::Erase(const std::string& key) {
    if (GetValueType() != JSON_OBJECT)
        return false;

    (*objectValue_)->remove(key);
	return true;
}

bool JSONValue::Contains(const std::string& key) const {
    if  (GetValueType() != JSON_OBJECT)
        return false;

    return (*objectValue_)->has(key);
}

JSONObjectIterator JSONValue::Begin() {
    // Convert to object type.
    SetType(JSON_OBJECT);

    return (*objectValue_)->begin();
}

ConstJSONObjectIterator JSONValue::Begin() const {
    if (GetValueType() != JSON_OBJECT)
        return (emptyObject)->begin();

    return (*objectValue_)->begin();
}

JSONObjectIterator JSONValue::End() {
    // Convert to object type.
    SetType(JSON_OBJECT);

    return (*objectValue_)->end();
}

ConstJSONObjectIterator JSONValue::End() const {
    if (GetValueType() != JSON_OBJECT)
        return emptyObject->end();

    return (*objectValue_)->end();
}

void JSONValue::Clear() {
    if (GetValueType() == JSON_ARRAY)
        (*arrayValue_)->clear();
    else if (GetValueType() == JSON_OBJECT)
        (*objectValue_)->clear();
}

void JSONValue::SetType(JSONValueType valueType) { //, JSONNumberType numberType) {
	std::cout << "SetType called." << std::endl;
	
    int type = valueType; // << 16u | numberType;
    if (type == type_)
        return;

	std::cout << "Deleting existing values..." << std::endl;

    switch (GetValueType()) {
    case JSON_STRING:
        delete stringValue_;
        break;

    case JSON_ARRAY:
        delete arrayValue_;
        break;

    case JSON_OBJECT:
        delete objectValue_;
        break;

    default:
        break;
    }

    type_ = type;
	
	std::cout << "Type is now set to: " << type_ << std::endl;

    switch (GetValueType()) {
    case JSON_STRING:
        stringValue_ = new std::string();
		
		std::cout << "New string created." << std::endl;
		
        break;

    case JSON_ARRAY:
        arrayValue_ = new JSONArray();
		
		std::cout << "New array created." << std::endl;
		
        break;

    case JSON_OBJECT:
        objectValue_ = new JSONObject();
		
		std::cout << "New object created." << std::endl;
		
        break;

    default:
        break;
    }
}

/* void JSONValue::SetVariant(const Variant& variant, Context* context) {
    if (!IsNull())
    {
        URHO3D_LOGWARNING("JsonValue is not null");
    }

    (*this)["type"] = variant.GetTypeName();
    (*this)["value"].SetVariantValue(variant, context);
}

Variant JSONValue::GetVariant() const
{
    VariantType type = Variant::GetTypeFromName((*this)["type"].GetString());
    return (*this)["value"].GetVariantValue(type);
}

void JSONValue::SetVariantValue(const Variant& variant, Context* context)
{
    if (!IsNull())
    {
        URHO3D_LOGWARNING("JsonValue is not null");
    }

    switch (variant.GetType())
    {
    case VAR_BOOL:
        *this = variant.GetBool();
        return;

    case VAR_INT:
        *this = variant.GetInt();
        return;

    case VAR_FLOAT:
        *this = variant.GetFloat();
        return;

    case VAR_DOUBLE:
        *this = variant.GetDouble();
        return;

    case VAR_STRING:
        *this = variant.GetString();
        return;

    case VAR_VARIANTVECTOR:
        SetVariantVector(variant.GetVariantVector(), context);
        return;

    case VAR_VARIANTMAP:
        SetVariantMap(variant.GetVariantMap(), context);
        return;

    case VAR_RESOURCEREF:
        {
            if (!context)
            {
                URHO3D_LOGERROR("Context must not be null for ResourceRef");
                return;
            }

            const ResourceRef& ref = variant.GetResourceRef();
            *this = String(context->GetTypeName(ref.type_)) + ";" + ref.name_;
        }
        return;

    case VAR_RESOURCEREFLIST:
        {
            if (!context)
            {
                URHO3D_LOGERROR("Context must not be null for ResourceRefList");
                return;
            }

            const ResourceRefList& refList = variant.GetResourceRefList();
            String str(context->GetTypeName(refList.type_));
            for (unsigned i = 0; i < refList.names_.Size(); ++i)
            {
                str += ";";
                str += refList.names_[i];
            }
            *this = str;
        }
        return;

    case VAR_STRINGVECTOR:
        {
            const StringVector& vector = variant.GetStringVector();
            Resize(vector.Size());
            for (unsigned i = 0; i < vector.Size(); ++i)
                (*this)[i] = vector[i];
        }
        return;

    default:
        *this = variant.ToString();
    }
}

Variant JSONValue::GetVariantValue(VariantType type) const
{
    Variant variant;
    switch (type)
    {
    case VAR_BOOL:
        variant = GetBool();
        break;

    case VAR_INT:
        variant = GetInt();
        break;

    case VAR_FLOAT:
        variant = GetFloat();
        break;

    case VAR_DOUBLE:
        variant = GetDouble();
        break;

    case VAR_STRING:
        variant = GetString();
        break;

    case VAR_VARIANTVECTOR:
        variant = GetVariantVector();
        break;

    case VAR_VARIANTMAP:
        variant = GetVariantMap();
        break;

    case VAR_RESOURCEREF:
        {
            ResourceRef ref;
            Vector<String> values = GetString().Split(';');
            if (values.Size() == 2)
            {
                ref.type_ = values[0];
                ref.name_ = values[1];
            }
            variant = ref;
        }
        break;

    case VAR_RESOURCEREFLIST:
        {
            ResourceRefList refList;
            Vector<String> values = GetString().Split(';', true);
            if (values.Size() >= 1)
            {
                refList.type_ = values[0];
                refList.names_.Resize(values.Size() - 1);
                for (unsigned i = 1; i < values.Size(); ++i)
                    refList.names_[i - 1] = values[i];
            }
            variant = refList;
        }
        break;

    case VAR_STRINGVECTOR:
        {
            StringVector vector;
            for (unsigned i = 0; i < Size(); ++i)
                vector.Push((*this)[i].GetString());
            variant = vector;
        }
        break;

    default:
        variant.FromString(type, GetString());
    }

    return variant;
}

void JSONValue::SetVariantMap(const VariantMap& variantMap, Context* context)
{
    SetType(JSON_OBJECT);
    for (VariantMap::ConstIterator i = variantMap.Begin(); i != variantMap.End(); ++i)
        (*this)[i->first_.ToString()].SetVariant(i->second_);
}

VariantMap JSONValue::GetVariantMap() const
{
    VariantMap variantMap;
    if (!IsObject())
    {
        URHO3D_LOGERROR("JSONValue is not a object");
        return variantMap;
    }

    for (ConstJSONObjectIterator i = Begin(); i != End(); ++i)
    {
        /// \todo Ideally this should allow any strings, but for now the convention is that the keys need to be hexadecimal StringHashes
        StringHash key(ToUInt(i->first_, 16));
        Variant variant = i->second_.GetVariant();
        variantMap[key] = variant;
    }

    return variantMap;
}

void JSONValue::SetVariantVector(const VariantVector& variantVector, Context* context)
{
    SetType(JSON_ARRAY);
    arrayValue_->Reserve(variantVector.Size());
    for (unsigned i = 0; i < variantVector.Size(); ++i)
    {
        JSONValue val;
        val.SetVariant(variantVector[i], context);
        arrayValue_->Push(val);
    }
}

VariantVector JSONValue::GetVariantVector() const
{
    VariantVector variantVector;
    if (!IsArray())
    {
        URHO3D_LOGERROR("JSONValue is not a array");
        return variantVector;
    }

    for (unsigned i = 0; i < Size(); ++i)
    {
        Variant variant = (*this)[i].GetVariant();
        variantVector.Push(variant);
    }

    return variantVector;
} */

std::string JSONValue::GetValueTypeName(JSONValueType type) {
    return valueTypeNames[type];
}

std::string JSONValue::GetNumberTypeName(JSONNumberType type) {
    return numberTypeNames[type];
}

JSONValueType JSONValue::GetValueTypeFromName(const std::string& typeName) {
    return GetValueTypeFromName(typeName.c_str());
}

/* JSONValueType JSONValue::GetValueTypeFromName(const char* typeName) {
    return (JSONValueType)GetStringListIndex(typeName, valueTypeNames, JSON_NULL);
} */

JSONNumberType JSONValue::GetNumberTypeFromName(const std::string& typeName) {
    return GetNumberTypeFromName(typeName.c_str());
}

/* JSONNumberType JSONValue::GetNumberTypeFromName(const char* typeName) {
    return (JSONNumberType)GetStringListIndex(typeName, numberTypeNames, JSONNT_NAN);
} */


void JSONValue::setVariant(Poco::Dynamic::Var var) {
	std::cout << "setVariant called." << std::endl;
	
	value = var;
	isObjectType = false;
	isArrayType = false;
	
	if (var.type() == typeid(Poco::JSON::Object::Ptr)) {
		std::cout << "Setting Object." << std::endl;
		objectPtr = value.extract<Poco::JSON::Object::Ptr>();
		isObjectType = true;
		isArrayType = false;
	}
	else if (var.type() == typeid(Poco::JSON::Array::Ptr)) {
		std::cout << "Setting Array." << std::endl;
		arrayPtr = value.extract<Poco::JSON::Array::Ptr>();
		isArrayType = true;
		isObjectType = false;
	}
}
