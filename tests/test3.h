#ifndef TEST_2_H
#define TEST_2_H

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
    long* secondMethod(int firstParameter);
    FirstClass aggregationToFirstClass;
};


#endif
