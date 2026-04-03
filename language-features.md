
# Language features

duty main() is the entry point


1. Statically typed
2. Object oriented-esque
3. Private/public methods
4. Immutability/mutability
5. Explicit clear names
6. Elixir's |> pipe symbol that takes the output of a function call into the
   input of the next function

    Underscore is placeholder for input

    string mut name = "go " |> String.duplicate(3, _) |> String.upcase(_) |> String.replace_suffix(" ", "!", _);

7. Imports, potentially from C, C++
8. Regular control flow, if , if-else, else if, switch, function, classes
9. Everything defaults to public

10. Cascade operator .. that allows multiple operations on the same object.
    Useful for setting up objects or calling several methods in a row.

    Greeter mut g = Greeter::("hello", 1) 
      ..num = 10 
      ..message = "hi" 
      ..greet()

11. Defer keyword that schedules code to run at the very end of the duty.
    Guarantees cleanup even if you return early or hit an error.

    duty read() do
        File mut f = File::open("test.txt")
        defer do 
            f.close()
        end

        if(f.is_empty()) do
            return 0
        end

        return 1
    end

12. Tuples & Multiple Returns.
    Duties can return more than one value, which is very purposeful for error handling 
    (status, value) without needing custom structs. 
    You can unpack them (destructure) instantly.
    duty get_user() -> (string, int) do
        return ("sam", 25)
    end
    pair (name, age) = get_user();

13. Crate ( struct);
- default to public

    visibility crate my_crate do
        string name;
        int num
    end;

# keywords

auto - types are inferred e.g. let mut name = "3.12"; this would be a double
immut
mut
private
public
duty
do - starts the control block e.g. duty, if, class, crate etc
end

match 
match-is
match-stop


# Variables
immutable
mutable
auto
must be explicit with type if not using auto
narrowing/widening conversion
auto is only allowed in variable initialisations


# function syntax

private/public function funtion_name                return type
visibility duty duty_name(string lang, int num) -> string do

end

# class syntax
default-pub/private
Similar c++ constructor initialization semantics


duty main() -> int do
    let immut user = "sam";
    let g = Greeter::("hello", 1);

    if(user == "sam") do
        print(g.num);
    end

end

visibility class Person do 

    private string language;
    int num;

    Person::(string lang, int num) : language(lang) do
        this.num = num;

    end

    Person::(end) //default constructor

end

string name = get_name();

match(name) do

    is "john": do
        stop;
    is "sam": do
        stop;

    default: do
        print("hello");

end


if(name == "sam") do
    print(name);

else if("name" == "bob") do
    print(name);
else do
    print("hello");

end


# for, while, might implement for-each

for(int i = 0;i < 10;i++) do


end


while(i < 10) do

end


for(Data d : list) do

end


# Arrays

    type name[size] = {};

    e.g. int my_arr[15] = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
    
    //similar to vector
    my_crate dynamic_list;

    dynamic_list.append({"sam", 1});
    print(dynamic_list.size());


