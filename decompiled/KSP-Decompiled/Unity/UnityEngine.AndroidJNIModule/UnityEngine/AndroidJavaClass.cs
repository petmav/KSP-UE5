using System;

namespace UnityEngine;

public class AndroidJavaClass : AndroidJavaObject
{
	public AndroidJavaClass(string className)
	{
		_AndroidJavaClass(className);
	}

	private void _AndroidJavaClass(string className)
	{
		DebugPrint("Creating AndroidJavaClass from " + className);
		IntPtr jobject = AndroidJNISafe.FindClass(className.Replace('.', '/'));
		m_jclass = new GlobalJavaObjectRef(jobject);
		m_jobject = new GlobalJavaObjectRef(IntPtr.Zero);
	}

	internal AndroidJavaClass(IntPtr jclass)
	{
		if (jclass == IntPtr.Zero)
		{
			throw new Exception("JNI: Init'd AndroidJavaClass with null ptr!");
		}
		m_jclass = new GlobalJavaObjectRef(jclass);
		m_jobject = new GlobalJavaObjectRef(IntPtr.Zero);
	}
}
