package main;

import main.MyClass;

public class Main {
    public static MyClass myObject = new MyClass("User");
    
    public static int hookMe(int myNumber, String myMessage) {
        System.out.println("My number is: " + myNumber);
        System.out.println("My message is: " + myMessage + " " + getPointer(myMessage));
        return myNumber * myNumber;
    }

    public native static long getAddress(Object obj);

    public static String getPointer(Object obj) {
        long addr = getAddress(obj);
        return Long.toHexString(addr);
    }
   
    public static void doStuff() {
        while (true) {
            int ret = hookMe(10, "Hello");
            System.out.println("hookMe return value: " + ret);
            System.out.println("myObject getUsername: " + myObject.getUsername());
            
            try {
                Thread.sleep(1000);
            } catch (Exception e) {
                System.out.println("bruh how???");
                break;
            }
        }
    }
    
    public static void main(String[] args) {
        System.out.println("My Java Program");

        System.loadLibrary("Main");
        
        if (args.length >= 1) {
            // Load library directly from Java process (good for testing with JNI_OnLoad)
            System.out.println("Library path: " + args[0]);
            System.loadLibrary(args[0]);
        }

        Main.doStuff();
    }
}
