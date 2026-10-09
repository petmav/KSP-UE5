using System.Collections.Generic;
using UnityEngine;

public static class ProfileTimer
{
	private class TimerInstance
	{
		public float start { get; private set; }

		public string name { get; private set; }

		public float duration => Time.realtimeSinceStartup - start;

		public TimerInstance(string name)
		{
			this.name = name;
			start = Time.realtimeSinceStartup;
		}
	}

	private static string timeFormat = "F6";

	private static List<TimerInstance> stack = new List<TimerInstance>();

	private static int stackCount = 0;

	private static void OutputTimer(TimerInstance timer)
	{
		Debug.Log("ProfileTimer " + timer.name + ": " + timer.duration.ToString(timeFormat) + "s");
	}

	public static void Push(string name)
	{
		stack.Add(new TimerInstance(name));
		stackCount++;
	}

	public static void Pop()
	{
		stackCount--;
		OutputTimer(stack[stackCount]);
		stack.RemoveAt(stackCount);
	}

	public static void Pop(string name)
	{
		int count = stack.Count;
		while (count-- > 0)
		{
			if (stack[count].name == name)
			{
				stackCount--;
				OutputTimer(stack[count]);
				stack.RemoveAt(count);
			}
		}
	}
}
