package com.liuwangyang.util;

import java.util.ArrayList;
import java.util.List;

import com.registry.RegStringValue;
import com.registry.RegistryKey;
import com.registry.RegistryValue;
import com.registry.ValueType;

//使用 JRegistry-1.8.1  方法和以前有区别          有不明白的可以联系我：QQ 605283073
public class newRegstryUtil 
{
	
	public static final int SUCCESS=0;//设置注册表成功
	public static final int CAN_NOT_WRITE=-1;//注册表项不可写
	public static final int NULL=87;//输入参数为空
	public static RegistryKey softWareKey=null;//本软件的 key
	public static RegStringValue regStringValue= null;//注册表字符串类型的值
	//获取"HKEY_LOCAL_MACHINE
	public static final RegistryKey LOCALMACHINE = RegistryKey.getRootKeyForIndex(RegistryKey.HKEY_LOCAL_MACHINE_INDEX);
	//获取"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"获取开机启动项key
	public static final RegistryKey RUN_KEY = new RegistryKey(LOCALMACHINE, "\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");


	/**
	 * 获取所有开机启动项
	 * @return 注册表(字符串)值的集合
	 */
	public  static List<RegStringValue> getAllRun() 
	{
	     List<RegistryValue> vregvalues =null;//注册表值
	     ArrayList<RegStringValue> regStringvalues = new ArrayList<RegStringValue> ();
	     
		//获取"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"
		if(RUN_KEY.hasValues())
		{	
		 vregvalues = RUN_KEY.getValues();	
		 
		 for(RegistryValue regvalue :vregvalues)
		 {
			 regStringvalues.add((RegStringValue)regvalue);
		 }
		}		
		 return regStringvalues;		
	}
	/**
	 * 获取指定开机启动项
	 * @return 注册表(字符串)值
	 */
	public  static RegStringValue getRun(String name) 
	{
		
		//获取"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"
		if(RUN_KEY.valueExists(name))
		{	
			regStringValue=(RegStringValue)RUN_KEY.getValue(name);
		}		
		return regStringValue;		
	}

	
	//值类型 valueType 开机启动项里  一般只用两种   1--> ValueType.REG_SZ 2-->ValueType.REG_EXPAND_SZ
	/**
	 * 设置开机启动项
	 * @param name 软件名
	 * @param value 软件路径(.exe 或者.jar结尾)
	 * @return  注册表该项不可写返回CAN_NOT_WRITE 成功返回：SUCCESS
	 */
	public static int setRun(String name,String value)//设置开机器启动项
	{

	
		
		if(RUN_KEY.canWrite())
		{
             if(RUN_KEY.valueExists(name))
             {
            	 RUN_KEY.deleteValue(name);
            	 
             }
             regStringValue= (RegStringValue)RUN_KEY.newValue(name, ValueType.REG_SZ);
        	 regStringValue.setValue(value);
        	 return  SUCCESS;
		}
			return  CAN_NOT_WRITE;
	
		
	}
	/**
	 *  删除开机启动项
	 * @param name 注册表项 下的名称
	 * @return    如果name为空 返回null否者返回成功
	 */
	public static int deleteRun(String name)
	{
		if ((name == null) || (name.length() == 0) || (name.equals("")))//保证传入参数不为空
		{
			return NULL;
		}else
		{
			if(RUN_KEY.canWrite())
			{
		      RUN_KEY.deleteValue(name);
		      return SUCCESS;
			}
			 return CAN_NOT_WRITE;
		}
		
		
	}

	
	
	
/**
 *  在软件项 下写入  键值对 （值名称 -- 值）
 * @param softwareName 软件名
 * @param name （该软件项下的）名称
 * @param value （该软件项下的）名称对应的值
 */
		public static void writeXmlPath(String softwareName,String name,String value)//在注册表中写入xml配置文件的路径
		{
		
		
			softWareKey = new RegistryKey(LOCALMACHINE, "SOFTWARE\\"+softwareName);
	        if(!softWareKey.exists())
	        {
	        	softWareKey.create();
	        	
	        }
	        regStringValue= (RegStringValue)RUN_KEY.newValue(name, ValueType.REG_SZ);
       	    regStringValue.setValue(value);
	      
			
		}
		
		
		/**
		 * 获取软件项下             名对应的值 
		 * @param softwareName 软件名
		 * @param keyName 名称
		 * @param value 值
		 * @return 值的字符串形式
		 */
		public static String readXmlPath(String softwareName,String name,String value)//在注册表中写入xml配置文件的路径
		{
			
			  softWareKey = new RegistryKey(LOCALMACHINE, "SOFTWARE\\"+softwareName);
			  if(softWareKey.valueExists(name))
			  {
				 regStringValue=  (RegStringValue)softWareKey.getValue(name); 
				 return regStringValue.getValue();
			  }
			
		      return  "";
		}
		
		/**
		 * 删除软件项下的键值
		 * @param softwareName
		 * @param name
		 */
		
		public static void deleteSoftwareValue(String softwareName,String name)
		{
			 softWareKey = new RegistryKey(LOCALMACHINE, "SOFTWARE\\"+softwareName);
			if(softWareKey.exists())
			{
				softWareKey.deleteValue(name);
			}
			
		}
	}

