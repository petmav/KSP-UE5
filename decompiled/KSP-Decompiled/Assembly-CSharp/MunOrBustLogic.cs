using UnityEngine;

public class MunOrBustLogic : MonoBehaviour
{
	[SerializeField]
	private Material targetMaterial;

	private void Start()
	{
		Texture2D texture = GameDatabase.Instance.GetTexture("Squad/MenuProps/MunOrBust", asNormalMap: false);
		if (texture != null)
		{
			targetMaterial.SetTexture("_MainTex", texture);
		}
	}
}
