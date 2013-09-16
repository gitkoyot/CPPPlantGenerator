
#include "test6.h"

SingletonClass* SingletonClass::me=0;

/*
@startuml
class SingletonClass{
me   :   SingletonClass *
}

SingletonClass o-- SingletonClass
@enduml
*/
