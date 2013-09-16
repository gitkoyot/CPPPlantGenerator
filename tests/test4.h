#ifndef TEST_4_H
#define TEST_4_H

class FirstClass
{
    int firstField;
    bool secondField;
    char* firstMethod();
    long* secondMethod(int firstParameter);
};



struct FirstStructure 
{
    int firstField;
    bool secondField;
    char* firstMethod();
    long* secondMethod( FirstClass dependencyToFirstClass );
};


#endif
