# C++
In most of my day to day programming I mainly use a small subset of C++
features. C++ is the best thing out there for many things as it offers
the programmer more control through low-level programming etc. For any
serious video-game engine there is not many options when it comes to
programming languages. Still I opt for not following the Modern C++ standard
and instead use a subset of C++ and this is why I want to document through
this markdown document my reasons for doing so.

Most comparisons will be to the C language but some comparisons will be made
with other low-level languages such as Go, D and Rust. I will attempt to
document both the good parts and the bad parts of C++ as well as collecting
resources for those who want to further investigate some of the topics 
that are discussed within this document.

In addition to documenting C++ features I want to collect code snippets
patterns and practices which could be overall useable for C++ programmers.
Some parts of this document will be highly oppinionated. For those parts I 
will try to add a disclaimer and keep the main content as information only.

## New things in C++
Compared to C, C++ brought a lot of new things to the table such as 
generics, lambdas, operator & function overloading, exceptions, RAII, const
correctness and a standard library. In this section I want to go through
some of the new things added in C++ which is not available in C.

### RAII
TODO
### Exceptions
TODO
### Templates
TODO
### Operator overloading / Function overloading
TODO
### Lambda
In C++ 11 and later the lambda expression was introduced. A lambda is a way
of defining an anonymous function object right at the location where it is
passed as an argument or invoked. Typically lambdas are used to encapsulate
a few lines of code that are passed to different algorithms or asynchronous
methods. Within this section I will attempt of describing lambdas, compare
them to other programming techniques as well as providing their advantages
and disadvantages.

Lambda is by now a well known concept both in programming and mathematics. 
For programming it is very common within functional programming languages, 
since C++ is a multi-paradigm language it makes sense to use functional 
programming techniques in some areas of your codebase to produce cleaner
and correct code. An attempt of this is anonymous functions which uses can
be very interesting which I will show later.

An outline of a lambda expression is as follows:
`[Captures] (Parameters) -> ReturnType { Statements; }` 
* Captures - The captures clause, also known as "Lambda introducer" specifies which outside variables are available for the lambda function and wether they should be captured by value (copy) or by reference. You will always be able to identify a start of a lambda expression with the presence of a capture clause. An empty capture clause "[]" means that the lambda expression body will not access variables in the enclosing scope.
* Parameters - This is the optional parameter list, also known as "Lambda declarator". This is optional and only dependant on if you want a function with parameters or a function that takes zero arguments.
* ReturnType - This is the return type of the lambda function. Most compilers can deduce the return type when you have zero or one return statement however for some the code will be easier to understand if the return type is clearly specified. 
* Statements - This is the lambda body. Theese statements can access the captured variables and the parameters. Think of this as a normal function body.

Following is a very simple example of a definition of a lambda expression 
named lambda that doesn't capture any variables, takes zero arguments and 
doesn't have a return statement. In this case the compiler will deduce that 
the return type is void and it isn't necessary to specify it. Within the
lambda body we have a simple printf statement which will write to the 
console. Notice that the capture clause starts the definition. Next the code
calls the defined lambda expression without arguments.
```C++
#include "stdio.h"

int main()
{
    auto lambda = []() { printf("This is a lambda expression\n"); };
    lambda();

    return (0);
}
```

Another very simple example of defining a lambda expression is the following
lambda expression named "sum" which does not capture any variables but takes
two arguments and returns an integer whose value is the sum of the two 
arguments. 
```C++
#include "stdio.h"

int main ()
{
    auto sum = [](int x, int y) { return x + y; };
    printf("%d\n", sum(10, 5));
    printf("%d\n", sum(2, 3));

    return (0);
}
```

So far we have had an empty capture clause but you can also use the default
capture mode "capture-default" to indicate how to capture any outside 
variables that are referenced in the lambda.
* `[&]` - Means that all variables you refer to are captured by reference.
* `[=]` - Means that all variables you refer to are captured by value.

For example, lets say that your lambda body accesses the external variable
`total` by reference and the external variable `factor` by value. Then
the following capture clauses are equivalent:
```C++
[&total, factor]
[factor, &total]
[&, factor]
[factor, &]
[=, &total]
[&total, =]
```
Only mentioned variables within the lambda are captured when a 
capture-default is used.

Now we have a better understanding of how lambdas are created, following 
are examples of use-cases for lambdas that could help make your life as 
a programmer easier or improve your code if you decide to make use of 
lambdas.

This following example is an example of using lambdas as a local initializer
function. This is an example of a fictional game engine that initializes 
a number of party members using a lambda helper function to do this work.
```C++
void InitializeParty()
{
    Array<PartyMember *> Members;

    auto MakePartyMember = [&Members](char *Name) 
    {
        auto Character = new PartyMember();
        Character->Name = CopyString(Name);
        Members.Add(Character);
    };

    MakePartyMember("John");
    MakePartyMember("Oskar");
    MakePartyMember("Alexandra");

    ...
}
```

What happens here is that we are declaring a lambda which is allowed to 
access the Array called Members by reference. It is then used to create
new party members and add them to the array of PartyMembers. If you know
how your initialization code should be done and successfully factored it 
then using a lambda like this could produce cleaner code with less paste 
errors than creating all the party members by hand. Normally you would want
to create a locally scoped function for this but this is not allowed in
C++ hence we make use of the lambda feature. While this produces clean code
there might be reasons you might not want to do this. In C++ performance 
would now be questionable. Some compilers may do heap allocations to store 
the data for the variables that the lambda captures which in this case is 
Members. Heap allocations is not an overhead you want in random locations of
your program so it is something to be aware of. 

A really interesting feature within the D language is the `scope(exit)` 
statement. This allows you to place your cleanup code next to your 
initialization code making it a lot easier to maintain and understand.
This is unfortunately not available in C++. Interesting however is that this
can easily be implemented using some new C++ features such as lambdas. 
In this case we want to define some code that is executed at the end of the 
scope to clean up objects allocated within the current scope. Using 
a lambda function that wraps the cleanup code this can easily be achieved 
when allocating an object on the stack who later invokes the lambda 
function. Take a look at the following example provided by The Witness blog:
```C++
#include <functional>

struct ScopeExit {
    ScopeExit(std::function f) : f(f) {}
    ~ScopeExit() { f(); }
    std::function f;
};

#define STRING_JOIN2(arg1, arg2) DO_STRING_JOIN2(arg1, arg2)
#define DO_STRING_JOIN2(arg1, arg2) arg1 ## arg2
#define SCOPE_EXIT(code) \
    ScopeExit STRING_JOIN2(scope_exit_, __LINE__)([=](){code;})
```

This would allow us as programmers to write code like this:

```C++
int * ip = new int[16];
SCOPE_EXIT(delete [] ip);

FILE * fp = fopen("test.out", "wb");
SCOPE_EXIT(fclose(fp));
```

There is libraries to mimic this behaviour as well but this example does
everything we wanted in less than 10 lines of code. Something interesting to
note here is the useage of `std::function`. A quick look at the assembly 
output confirms that the generated code is awfull. It would make more sense
to wrap the closure explicitly and reuly on type inference to allocate the
wrapper object on the stack. This means we could change the code to the 
following:
```C++
template <typename F>
struct ScopeExit {
    ScopeExit(F f) : f(f) {}
    ~ScopeExit() { f(); }
    F f;
};

template <typename F>
ScopeExit<F> MakeScopeExit(F f) {
    return ScopeExit<F>(f);
};

#define SCOPE_EXIT(code) \
    auto STRING_JOIN2(scope_exit_, __LINE__) = MakeScopeExit([=](){code;})
```

If you as a programmer want to use this feature or not is up to you. 
Myself I would like to investigate the performance overhead of using lambdas
compared to regular functions before diving in deeper and making use of 
them. But we can clearly see within this section that lambdas can produce
very clean and interesting pieces of code. In my oppinion even if you 
dot not want to use it in your own everyday waork they should be available
in your programming toolbox if ever needed.

## Reading resources for C/C++
This part of the document will contain a list of recomended reading 
resources for those interesting in learning more about the language as well
as different areas of programming relative to what I'm working with.

* The C Programming Language - Brian Kernighan & Dennis Ritchie
* The C++ Programming Language - Bjarne Stroustrup
* Effective C++ - Scott Meyers
* Structure and Interpretation of Computer Programs - Harold Abelson & Gerald Jay Sussman

## References
* [Microsoft docs: Lambda Expressions in C++](https://docs.microsoft.com/en-us/cpp/cpp/lambda-expressions-in-cpp?view=vs-2019)
* [Dr.Dobbs Lambdas in C++11](http://www.drdobbs.com/cpp/lambdas-in-c11/240168241?pgno=2)
* [scope(exit) in C++11](http://the-witness.net/news/2012/11/scopeexit-in-c11/)