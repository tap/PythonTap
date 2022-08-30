from time import time,ctime

print("This is a test -- Booyah!")
print('Today is', ctime(time()))

def foo(arg1):
    print("foo foo on yo do do")
    print(f"arg: {arg1}")
    return arg1 + 1

def bar():
    print("bar bar on yo dar dar")
    return 74
