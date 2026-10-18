
import java.io.*;
import java.net.*;
import java.util.Date;

class ServerWorker extends Thread {
    private final Socket clientSocket;
    private final BufferedReader in;
    private final PrintWriter printWriter;
    private final DataInputStream dataIn;
    private final DataOutputStream dataOut;
    private final static int chunkSize = 32;


    public ServerWorker(Socket clientSocket) throws IOException {
        this.clientSocket = clientSocket;
        this.in = new BufferedReader(new InputStreamReader(clientSocket.getInputStream()));
        this.printWriter = new PrintWriter(clientSocket.getOutputStream(), true);
        this.dataIn = new DataInputStream(clientSocket.getInputStream());
        this.dataOut = new DataOutputStream(clientSocket.getOutputStream());
        writeLogFile("Connection received\n");
    }

    public void writeLogFile(String str){
        try {
            FileWriter fileWriter = new FileWriter("log.txt", true);
            fileWriter.write(str);
            fileWriter.close();

        } catch (IOException e) {
            e.printStackTrace();
        }
    }


    @Override
    public void run() {
        try {
            // read data from the client and send a response
            String input = in.readLine();
            System.out.println("input : " + input);
            writeLogFile("input : " + input + "\n");

            if (input == null)
                return;
            if (input.length() > 0) {
                if (input.startsWith("GET"))
                    handleGetReq(input);
                else if (input.startsWith("UPLOAD"))
                    handleUploadReq(input);
                else {
                    System.out.println("Invalid request");
                    sendResponse("400 Bad Request");
                }
            }

        } catch (IOException e) {
            e.printStackTrace();
        }

        finally {
            try {
                Thread.sleep(2000);
                clientSocket.close();
                System.out.println("Connection closed");
                System.out.println();
            } catch (IOException e) {
                e.printStackTrace();
            } catch (InterruptedException e) {
                throw new RuntimeException(e);
            }
        }
    }


    public void handleGetReq(String input) {
        StringBuilder response = new StringBuilder();
        String[] tokens = input.split(" ");
        String filePath = tokens[1];
        try {
            if (filePath.endsWith(".docx") || filePath.endsWith(".txt") || filePath.endsWith(".mp4") || filePath.endsWith(".pdf") || filePath.endsWith(".jpeg") || filePath.endsWith(".jpg") || filePath.endsWith(".png")) {
                File file = new File(filePath.substring(1));

                if (file.exists()) {
                    int fileLength = (int) file.length();
                    response.append(getResponseHeader(200, "OK", getContentType(filePath), fileLength));
                    sendResponse(response.toString());
                    writeLogFile("\nResponse : \n" + response);

                    // send file data
                    FileInputStream fileIn = new FileInputStream(file);
                    byte[] fileData = new byte[chunkSize];
                    int bytes;
                    while ((bytes = fileIn.read(fileData)) != -1) {
                        dataOut.write(fileData, 0, bytes);
                        dataOut.flush();
                    }
                }
            }
            else {
                // directory is requested, showing list of files
                String content = getHTMLContent(filePath);
                response.append(getResponseHeader(200, "OK", "text/html", content.length())).append(content);
                sendResponse(response.toString());
                writeLogFile("\nResponse : \n" + response);
                System.out.println("200 OK");
            }

        } catch (FileNotFoundException e) {
            response.append(getResponseHeader(404, "Not Found", "", 0));
            sendResponse(response.toString());
            writeLogFile("\nResponse : \n" + response);
            System.out.println("404 Not Found");
        } catch (Exception e) {
            // Handle any other exceptions
            e.printStackTrace();
        }
    }


    // handle UPLOAD request
    public void handleUploadReq(String input) {
        StringBuilder response = new StringBuilder();
        long fileSize = -1;
        String[] tokens = input.split(" ", 2);
        if (tokens.length != 2) {
            sendResponse("ERROR");
            writeLogFile("\nResponse : ERROR\n");
            return;
        }
        // write was not working, so println was used
        printWriter.println("OK");
        printWriter.flush();
        System.out.println("Acknowledge sent");
        String fileName = tokens[1];

        try {
            if (in.readLine().startsWith("UPLOADING ")) {
                // above received string is indicator of data stream starting
                int bytes;
                FileOutputStream fileOut = new FileOutputStream("client/" + fileName);
                fileSize = dataIn.readLong(); // read file size
                System.out.println("fileSize : " + fileSize + " bytes");
                byte[] fileData = new byte[chunkSize];
                while (fileSize > 0 &&
                        (bytes = this.dataIn.read(fileData, 0, (int) Math.min(fileData.length, fileSize))) != -1) {
                    fileOut.write(fileData, 0, bytes);
                    fileSize -= bytes;
                }
                fileOut.close();
                System.out.println("Upload complete");
            }
            else
                System.out.println("Upload failed");


        } catch (IOException e) {
            e.printStackTrace();
        }

        finally {
            if (fileSize == 0) {
                response.append(getResponseHeader(200, "OK", "", 0));
                System.out.println("200 OK");
            }
            else {
                response.append(getResponseHeader(417, "Upload failed", "", 0));
                System.out.println("417 Upload failed");
            }
            sendResponse(response.toString());
            writeLogFile("\nResponse : \n" + response);
        }
    }


    public String getResponseHeader(int statusCode, String statusMsg, String type, int length) {
        StringBuilder response = new StringBuilder();
        String lineSeparator = System.lineSeparator();

        response.append("HTTP/1.0 ").append(statusCode).append(" ").append(statusMsg).append(lineSeparator);
        response.append("Server: Java HTTP Server: 1.0").append(lineSeparator);
        response.append("Date: ").append(new Date()).append(lineSeparator);
        response.append("Content-Type: ").append(type).append(lineSeparator);
        response.append("Content-Length: ").append(length).append(lineSeparator);
        response.append(lineSeparator);

        return response.toString();
    }



    public void sendResponse(String response) {
        printWriter.write(response);
        printWriter.flush();
    }

    public String getContentType(String fileName) {
        String type;
        if(fileName.endsWith(".html"))
            type = "text/html";
        else if(fileName.endsWith(".jpg") || fileName.endsWith(".jpeg"))
            type = "image/jpeg";
        else if(fileName.endsWith(".png"))
            type = "image/png";
        else if(fileName.endsWith(".mp4"))
            type = "video/mp4";
       else if(fileName.endsWith(".pdf"))
           type = "application/pdf";
        else if(fileName.endsWith(".txt"))
            type = "text/plain";
        else
            type = "application/octet-stream"; // this allows for the file to be downloaded instead of opened

        return type;
    }

    public String getHTMLContent(String fileName) throws FileNotFoundException {
        StringBuilder html = new StringBuilder();

        // home directory
        if(fileName.equals("/"))
            return "<h1>File Server/</h1><ul><li><b><i><a href=\"root\">root</a></i></b></li><li><b><i><a href=\"client\">client</a></i></b></li></ul>";

        File folder = new File(fileName.substring(1));

        if(folder.exists()) {
            File[] listOfFiles = folder.listFiles();
            html.append("<h1>File Server").append(fileName).append("</h1>");
            if(listOfFiles == null)
                return html + "<p>Empty directory</p>";

            html.append("<ul>");

            // make list of the files in the directory
            // bold and italic for directories
            for (File file : listOfFiles) {
                if(file.isDirectory())
                    html.append("<li><b><i><a href=\"").append(fileName).append("/").append(file.getName()).append("\">").append(file.getName()).append("</a></i></b></li>");
                else
                    html.append("<li><a href=\"").append(fileName).append("/").append(file.getName()).append("\">").append(file.getName()).append("</a></li>");
            }
            html.append("</ul>");
            return html.toString();
        }
        else
            throw new FileNotFoundException();
    }
}
