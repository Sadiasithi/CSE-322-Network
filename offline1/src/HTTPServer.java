
import java.io.*;
import java.net.*;

public class HTTPServer {
    static final int PORT = 5119;

    public static void main(String[] args) {

        try (ServerSocket serverSocket = new ServerSocket(PORT)) {
            System.out.println("Server started.");
            System.out.println("Listening for connections on port : " + PORT + " ...\n");

            while (true) {
                // Listen for incoming client connections
                Socket clientSocket = serverSocket.accept();
                System.out.println("Connection received");

                // Create a new thread for the client
                Thread serverWorker = new ServerWorker(clientSocket);
                serverWorker.start();
            }
        }
        catch (IOException e) {
            e.printStackTrace();
        }
    }
}
