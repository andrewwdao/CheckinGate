import java.util.*;
import java.io.File;

public class DeleteFileByName{
	public static void main(String []args){
		// Max file in derectory
		int maxFileInDerectory = 5;
		
		// Path to derectory
		String derectoryPath = "./Test/";
		
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
}
