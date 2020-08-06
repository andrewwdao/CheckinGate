import java.util.*;
import java.io.File;

public class DeleteFileByNameFunction{
    // Define function with Max file number in derectory and Path to derectory args
    static void deleteFileByName(int maxFileInDerectory, String derectoryPath) {
        // Max file in derectory
		
		// Path to derectory
		
		// List file in Directory
		File[] files = new File(derectoryPath).listFiles();
		
		int count=0;
    	for (File file : files){
        	count++;
        	System.out.println(file);
        	if(count >= maxFileInDerectory){
                	file.delete();
        	}
    	}
    }
    
    //Main test
	public static void main(String[] args) {
		deleteFileByName(10, "./Test/");
	}
}
