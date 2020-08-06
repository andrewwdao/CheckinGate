import java.util.*;
import java.io.File;

public class DeleteFileByName{
	public static void main(String []args){
		// Max file in directory
		int maxFileInDirectory = 5;
		
		// Path to directory
		String directoryPath = "./Test/";
		
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
}
