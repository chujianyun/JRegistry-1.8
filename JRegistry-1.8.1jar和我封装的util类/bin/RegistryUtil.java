package com.mingmingruyue.registry;


import java.util.List;

import com.registry.RegStringValue;
import com.registry.RegistryKey;
import com.registry.RegistryValue;
import com.registry.ValueType;

//封装实现注册表相关操作
public class Registry 
{
	public static final int SUCCESS=0;//设置注册表成功
	public static final int CAN_NOT_WRITE=-1;//注册表项不可写
	public static final int NULL=87;//输入参数为空
	
	//获取"HKEY_LOCAL_MACHINE
	public static final RegistryKey LOCALMACHINE = RegistryKey.getRootKeyForIndex(RegistryKey.HKEY_LOCAL_MACHINE_INDEX);
	//获取"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"获取开机启动项key
	public static final RegistryKey RUN_KEY = new RegistryKey(LOCALMACHINE, "\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run");
	
	
	
	private RegStringValue regValue= null;//注册表字符串类型的值
	private List<RegistryValue> values =null;//注册表值
	private RegistryKey runRegistryKey =null;//注册表key

	//获取开机启动项
	public  List<RegistryValue> getRun() 
	{
		//获取"HKEY_LOCAL_MACHINE\\SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Run"
		if(RUN_KEY.hasValues())
		{	
		 values = RUN_KEY.getValues();		
		}		
		 return values;		
	}

	
	//值类型 valueType 开机启动项里  一般只用两种   1--> ValueType.REG_SZ 2-->ValueType.REG_EXPAND_SZ
	public int setRun(String name,String value)//设置开机器启动项
	{
		int result=-1;
        
		regValue =(RegStringValue)RUN_KEY.newValue(name,ValueType.REG_SZ );//创建注册表项{名称}
		if(RUN_KEY.canWrite())
		{
		
				
				regValue.setValue(value);//{数据值}
			    
			    regValue.refreshData();
		   result=SUCCESS;
		}else
		{
			result=CAN_NOT_WRITE;
		}
		return result;
	}
	
	public int deleteRun(String name)//删除开机启动项
	{
		if ((name == null) || (name.length() == 0) || (name.equals("")))//保证传入参数不为空
		{
			return NULL;
		}else
		{
		  RUN_KEY.deleteValue(name);
		  return SUCCESS;
		}
		
		
	}

}
