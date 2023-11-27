package main;

public class Main {
    public static int hookMe(int myNumber) {
        System.out.println("My number is: " + myNumber);
        return myNumber * myNumber;
    }
   
    public static void doStuff() {
        while (true) {
            int ret = hookMe(10);
            System.out.println("hookMe return value: " + ret);
            
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
        
        if (args.length >= 1) {
            // Load library directly from Java process (good for testing with JNI_OnLoad)
            System.out.println("Library path: " + args[0]);
            System.load(args[0]);
        }

        Main.doStuff();
    }
}
