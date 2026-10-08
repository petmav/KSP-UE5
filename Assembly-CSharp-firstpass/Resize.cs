using System;
using UnityEngine;

public class Resize
{
	public static void Array(ref Vector2[] array, int newSize)
	{
		System.Array.Resize(ref array, newSize);
	}
}
