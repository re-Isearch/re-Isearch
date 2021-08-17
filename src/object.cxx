/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
//
// Object.cc
//

#include "object.hxx"


//***************************************************************************
// Object::Object()
//
Object::Object() { }


//***************************************************************************
// Object::~Object()
//
Object::~Object() { }


//***************************************************************************
// int Object::Compare(const Object&) const
//
int Object::Compare(const Object&) const { return 0; }


//***************************************************************************
// Object *Object::Clone() const
//
Object *Object::Clone() const { return new Object; }


