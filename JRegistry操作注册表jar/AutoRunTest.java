package com.liuwangyang.test;
import java.io.File;

import com.liuwangyang.util.AutoRunUtil;


public class AutoRunTest 
{
	
	public static void main(String [] args)
	{
		
	/*	//设置开启启动项
	   String softwareName1 ="filetransport";
		File file1 = new File("D:\\Program Files\\文件传输工具3.0.jar");
	
		
		String softwareName2="word&pdf2txt1.5";
		File file2 = new File("D:\\Program Files\\word&pdf2txt1.5.exe");
		
		AutoRunUtil.setRun(softwareName1, file1.getPath());
		AutoRunUtil.setRun(softwareName2, file2.getPath());*/
		//删除开机启动项
	  /* AutoRunUtil.deleteRun("word&pdf2txt1.5");*/
		//设置软件的注册表项极其配置文件和路径
		
		//设置XML文件路径
	/*	File file = new File("C:\\UpdateList.xml");
		AutoRunUtil.writeXmlPath("autoBackups", "xmlPath", file.getAbsolutePath());*/
		
		//删除XML文件的键值
	/*	AutoRunUtil.deleteSoftwareValue("autoBackups", "xmlPath");*/
		//删除软件项
		AutoRunUtil.deleteSoftwareKey("autoBackups");
		
	}

}
