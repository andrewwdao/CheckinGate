import java.util.*;
import java.io.File;

public class DeleteFileByNameFunction{
    // Define function with Max file number in directory and Path to directory args
    static void deleteFileByName(int maxFileInDirectory, String directoryPath) {
	// List file in Directory
	File[] files = new File(directoryPath).listFiles();

	int count=0;
    	for (File file : files){
        	count++;
        	System.out.println(file);
        	if(count >= maxFileInDirectory){
                	file.delete();
        	}
    	}
    }
    
    //Main test
	public static void main(String[] args) {
		deleteFileByName(10, "./Test/");
	}
}
