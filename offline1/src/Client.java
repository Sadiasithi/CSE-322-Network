
import java.io.IOException;
import java.util.Scanner;

public class Client {
    public static void main(String[] args) {
        Scanner scn = new Scanner(System.in);
        try {
            while(true) {
                Thread clientWorker = new ClientWorker(scn.nextLine());
                clientWorker.start();
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }
}
