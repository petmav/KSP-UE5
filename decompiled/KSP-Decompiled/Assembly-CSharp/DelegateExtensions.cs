using System;

public static class DelegateExtensions
{
	public static void Clear<T>(this Func<T> f, T defaultValue)
	{
		f = () => defaultValue;
	}

	public static void Clear<T, R>(this Func<T, R> f, R defaultValue)
	{
		f = (T t) => defaultValue;
	}

	public static void Clear<T, U, R>(this Func<T, U, R> f, R defaultValue)
	{
		f = (T t, U u) => defaultValue;
	}

	public static void Clear<T, U, V, R>(this Func<T, U, V, R> f, R defaultValue)
	{
		f = (T t, U u, V v) => defaultValue;
	}

	public static void Clear<T, U, V, W, R>(this Func<T, U, V, W, R> f, R defaultValue)
	{
		f = (T t, U u, V v, W w) => defaultValue;
	}
}
