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

#pragma once

#include "jsonvalue.h"


#include <string>


#include <angelscript.h>


#include <Poco/JSON/Parser.h>
#include <Poco/Dynamic/Var.h>


template <class T> T* ConstructObject() {
    auto* object = new T();
    object->AddRef();
    return object;
}

template <class T> T* ConstructNamedObject(const std::string& name) {
    auto* object = new T();
    object->AddRef();
    object->SetName(name);
    return object;
}


void RegisterJSONFile(asIScriptEngine* engine);


/// JSON document resource.
class JSONFile {

public:
    /// Construct.
    explicit JSONFile();
    /// Destruct.
    ~JSONFile();
    /// Register object factory.
    //static void RegisterObject(Context* context);

    /// Load resource from stream. May be called from a worker thread. Return true if successful.
    //bool BeginLoad(Deserializer& source) override;
    /// Save resource with default indentation (one tab). Return true if successful.
    //bool Save(Serializer& dest) const override;
    /// Save resource with user-defined indentation, only the first character (if any) of the string is used and the length of the string defines the character count. Return true if successful.
    //bool Save(Serializer& dest, const std::string& indendation) const;

    /// Deserialize from a string. Return true if successful.
    bool fromString(const std::string& source);
    /// Save to a string.
    std::string toString(const std::string& indentation = "\t") const;

    /// Return root value.
    JSONValue& getRoot() { return root_; }
    /// Return root value.
    const JSONValue& getRoot() const { return root_; }

private:
    /// JSON root value.
    JSONValue root_;
};
