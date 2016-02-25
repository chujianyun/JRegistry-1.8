package com.liuwangyang.util;
import ca.beq.util.win32.registry.RegistryKey;
import ca.beq.util.win32.registry.RegistryValue;
import ca.beq.util.win32.registry.RootKey;
import ca.beq.util.win32.registry.ValueType;


public class AutoRunUtil 
{
	//获取"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"获取开机启动项
	public static RegistryKey RUN = new RegistryKey(RootKey.HKEY_LOCAL_MACHINE,  "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");  
   //HKEY_LOCAL_MACHINE\SOFTWARE  软件基本在这里
	public static RegistryKey SOFTWARE = new RegistryKey(RootKey.HKEY_LOCAL_MACHINE,  "SOFTWARE");  
	public static RegistryKey softWareKey=null;//本软件的 key
	/**设置开机启动项
	 * 
	 * @param name 名称（软件英文名）
	 * @param value 数据{文件路径=最好是英文}
	 */
	
	public static  void setRun(String name , String value)

	{
		RegistryValue v = new RegistryValue(name, ValueType.REG_SZ, value);  
		
		RUN.setValue(v);	
	}
	
	/**
	 * 删除开机启动项
	 * @param name 名称（软件英文名）
	 */
	public static void deleteRun(String name)
	{
	
		RUN.deleteValue(name);		
	}
/**
 *  判断软件注册表项是否存在不存在即创建
 * @param r
 * @param softwareName
 * @param path
 */
	
	public static void writeXmlPath(String softwareName,String keyName,String value)//在注册表中写入xml配置文件的路径
	{
	
		softWareKey = new RegistryKey(RootKey.HKEY_LOCAL_MACHINE, "SOFTWARE\\"+softwareName);
        if(!softWareKey.exists())
        {
        	softWareKey.create();
        	
        }
        
        RegistryValue v = new RegistryValue(keyName, ValueType.REG_SZ, value);  
        softWareKey.setValue(v);
		
	}
	
	/**
	 * 获取软件XML的路径
	 * @param softwareName 软件名
	 * @param keyName 名称
	 * @param value 值
	 * @return 值的字符串形式
	 */
	public static String readXmlPath(String softwareName,String keyName,String value)//在注册表中写入xml配置文件的路径
	{
		
		  softWareKey = new RegistryKey(RootKey.HKEY_LOCAL_MACHINE, "SOFTWARE\\"+softwareName);
		  if(softWareKey.hasValue(keyName))
		  {
			  return  softWareKey.getValue(keyName).getStringValue(); 
		  }
		
	      return  "";
	}
	/**
	 * 删除软件的项
	 * @param softwareName 软件名
	 */
	public static void deleteSoftwareKey(String softwareName)
	{
		softWareKey = new RegistryKey(RootKey.HKEY_LOCAL_MACHINE, "SOFTWARE\\"+softwareName);
		if(softWareKey.exists())
		{
			softWareKey.delete();
		}
		
	}
	/**
	 * 删除软件项下的键值
	 * @param softwareName
	 * @param name
	 */
	
	public static void deleteSoftwareValue(String softwareName,String name)
	{
		softWareKey = new RegistryKey(RootKey.HKEY_LOCAL_MACHINE, "SOFTWARE\\"+softwareName);
		if(softWareKey.exists())
		{
			softWareKey.deleteValue(name);
		}
		
	}
}
