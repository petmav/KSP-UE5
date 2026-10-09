namespace Smooth.Delegates
{
	public static class DelegateExtensions
	{
		public static void Apply<T>(this T t, DelegateAction<T> a)
		{
			a(t);
		}

		public static void Apply<T, P>(this T t, DelegateAction<T, P> a, P p)
		{
			a(t, p);
		}

		public static U Apply<T, U>(this T t, DelegateFunc<T, U> f)
		{
			return f(t);
		}

		public static U Apply<T, U, P>(this T t, DelegateFunc<T, P, U> f, P p)
		{
			return f(t, p);
		}
	}
}
