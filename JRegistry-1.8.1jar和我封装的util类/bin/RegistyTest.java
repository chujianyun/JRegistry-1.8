package com.mingmingruyue.registry;

import java.util.Iterator;
import java.util.List;

import com.registry.RegStringValue;
import com.registry.RegistryKey;
import com.registry.RegistryValue;

public class RegistyTest {

	public static void main(String args[]) 
	{

		Registry regstry = new Registry();
		RegStringValue regValue = null;//注册表字符串类型的值

		// -------------------获取开机启动项目----------------------------
/*		List<RegistryValue> values = regstry.getRun();
       if(!values.isEmpty())
     {
		System.out.println("-----------开机启动项：------------------");
		Iterator<RegistryValue> it = values.iterator();
		while (it.hasNext()) {
			regValue = (RegStringValue) it.next();
			// new String(regValue.getByteData(),"UNICODE")

			System.out.println("名称：-->" + regValue.getName() + "  类型:--> "
					+ regValue.getValueType() + "   数值:--> "
					+ regValue.getValue());
		}

		System.out.println("-----------开机启动项：--------------------");
		
     }else
     {
    	 
    	 System.out.println("-----------没有开机启动项------------");
     }*/
		
		// -------------------设置开机启动项目----------------------------
/*      
       regstry.setRun("hideandbutify", "D:\\Program Files\\hideandbutify.exe");
       System.out.println("-----------设置开机启动项--完毕----------");*/
		
		RegistryKey reg = new RegistryKey(regstry.RUN_KEY, "hideandbutify");
		System.out.println(reg.canDelete()+reg.toString());
		reg.deleteLink();
		
		
		
	}
}
