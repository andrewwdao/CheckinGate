import java.io.IOException;
import java.util.*;
import java.io.File;
import java.nio.file.attribute.BasicFileAttributes;
import java.nio.file.Files;
import java.nio.file.Paths;

public class DeleteFileByCreationTime{
	public static void main(String []args){
		// Max file in derectory
		int maxFileInDerectory = 5;
		
		// Path to derectory
		String derectoryPath = "./Test/";
		
		// List file in Directory
		File[] files = new File(derectoryPath).listFiles();
		
		// Define Comparator by creationTime attribute of File
		Comparator<File> comparator = Comparator.comparing(file -> {
        		try {
                		return Files.readAttributes(Paths.get(file.toURI()), BasicFileAttributes.class).creationTime();
        		} catch (IOException e) {
                		return null;
        		}
        	});

		// Sort file by creation time
        	Arrays.sort(files, comparator);
		
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
