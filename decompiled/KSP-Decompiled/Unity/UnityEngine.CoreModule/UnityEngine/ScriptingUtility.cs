using UnityEngine.Scripting;

namespace UnityEngine;

internal class ScriptingUtility
{
	private struct TestClass
	{
		public int value;
	}

	[RequiredByNativeCode]
	private static bool IsManagedCodeWorking()
	{
		TestClass testClass = default(TestClass);
		testClass.value = 42;
		TestClass testClass2 = testClass;
		return testClass2.value == 42;
	}
}
