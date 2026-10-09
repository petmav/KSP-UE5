using UnityEngine;

namespace Assets._UI5.Rendering.Scripts
{
	public class ImgFX_ColorMultiply : MonoBehaviour
	{
		public Color imgColor = Color.white;

		private Material mat;

		private int dstTexID;

		[SerializeField]
		private Shader shader;

		protected void Awake()
		{
			mat = new Material(shader);
			dstTexID = Shader.PropertyToID("_DstTex");
		}

		public void OnRenderImage(RenderTexture source, RenderTexture destination)
		{
			if (imgColor == Color.white)
			{
				Graphics.Blit(source, destination);
				return;
			}
			mat.color = imgColor;
			mat.SetTexture(dstTexID, destination);
			Graphics.Blit(source, destination, mat);
		}
	}
}
