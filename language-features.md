
# Language features

duty main() is the entry point


1. Statically typed
2. Object oriented-esque
3. Private/public methods
4. Immutability/mutability
5. Explicit clear names
6. Elixir's |> pipe symbol that takes the output of a function call into the
   input of the next function
result = "go " |> String.duplicate(3) |> String.upcase()|> String.replace_suffix(" ", "!")
7. Imports, potentially from C, C++
8. Regular control flow, if , if-else, else if, switch, function, classes
9. Everything defaults to public

# keywords

auto - types are inferred e.g. let mut name = "3.12"; this would be a double
immut
mut
prv
pub
duty
do - starts the control block e.g. duty, if, class etc
end class
end duty
end if
end 'constructor name'

match 
match-is
match-stop


# Variables
immutable
mutable
auto
must be explicit with type if not using auto
narrowing/widening conversino
auto is only allowed in variable initialisations


# function syntax

private/public function funtion_name                return type
visibility duty duty_name(string lang, int num) -> string do

end duty

# class syntax
default-pub/private
Similar c++ constructor initialization semantics


duty main() -> int do
    let immut user = "sam";
    let g = Greeter::("hello", 1);

    if(user == "sam") do
        print(g.num);
    end if

end duty

visibility class Person do 

    private string language;
    int num;

    Person::(string lang, int num) : language(lang) do
        this.num = num;

    end Person

    Person::(end) //default constructor

end class

string name = get_name();

match(name) do

    is "john" do
        stop;
    is "sam" do
        stop;

    default do
        print("hello");

end match


if(name == "sam") do
    print(name);

else if("name" == "bob") do
    print(name);
else do
    print("hello");

end if


# for, while, might implement for-each

for(int i = 0;i < 10;i++) do


end for


while(i < 10) do

end while


for(Data d : list) do

end for

