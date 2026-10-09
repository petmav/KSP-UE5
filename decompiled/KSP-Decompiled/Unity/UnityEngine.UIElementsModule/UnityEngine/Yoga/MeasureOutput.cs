namespace UnityEngine.Yoga;

internal class MeasureOutput
{
	public static YogaSize Make(float width, float height)
	{
		YogaSize result = default(YogaSize);
		result.width = width;
		result.height = height;
		return result;
	}
}
