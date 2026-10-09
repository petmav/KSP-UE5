using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text.RegularExpressions;
using UnityEngine;

public class OBJ : MonoBehaviour
{
	private class scaleoffset
	{
		public string name;

		public bool initval;

		public float MasterScale = 1f;

		public float minvpos;

		public float maxvpos;

		public float minvxpos;

		public float maxvxpos;

		public float minvypos;

		public float maxvypos;

		public float minvzpos;

		public float maxvzpos;

		public float MasterOffsetx;

		public float MasterOffsety;

		public float MasterOffsetz;

		public Vector3 MainOffset;

		public int vfirst;

		public int vlast;
	}

	private class MaterialData
	{
		public string name;

		public Color ambient;

		public Color diffuse;

		public Color specular;

		public Color emmisive;

		public bool emmision;

		public float shininess;

		public float alpha = 1f;

		public int illumType;

		public string ambientTexPath;

		public string diffuseTexPath;

		public string emmisiveTexPath;

		public string specularTexPath;

		public string alphaTexPath;

		public string bumpTexPath;

		public Texture2D DiffTexture;

		public Texture2D BumpTexture;

		public Texture2D EmmisiveTexture;

		public string ShaderName;
	}

	public string objPath;

	public bool MakeCollider;

	public bool SaveToProject;

	public float objmaxsize = 1000f;

	public float objminsize = 0.01f;

	public bool EnforceSingleObj;

	public float BumpStrength = 1.8f;

	private scaleoffset mainso;

	private scaleoffset currentso;

	private List<scaleoffset> objso = new List<scaleoffset>();

	private bool treatasoneobject;

	private int vcount;

	private const string O = "o";

	private const string G = "g";

	private const string V = "v";

	private const string VT = "vt";

	private const string VN = "vn";

	private const string F = "f";

	private const string MTL = "mtllib";

	private const string UML = "usemtl";

	private const string NML = "newmtl";

	private const string NS = "Ns";

	private const string KA = "Ka";

	private const string KD = "Kd";

	private const string KE = "Ke";

	private const string KS = "Ks";

	private const string D = "d";

	private const string TR = "Tr";

	private const string ILLUM = "illum";

	private const string MAP_KA = "map_Ka";

	private const string MAP_KD = "map_Kd";

	private const string MAP_KS = "map_Ks";

	private const string MAP_KE = "map_Ke";

	private const string MAP_D = "map_d";

	private const string MAP_TR = "map_tr";

	private const string MAP_BUMP = "map_bump";

	private string basepath;

	private string mtllib;

	private GeometryBuffer buffer;

	private Texture2D[] TMPTextures;

	private Dictionary<string, int> TextureList = new Dictionary<string, int>();

	private List<MaterialData> materialData;

	private bool hasMaterials => mtllib != null;

	private void Start()
	{
		buffer = new GeometryBuffer();
		if (objPath != "")
		{
			if (!objPath.Contains("http://"))
			{
				basepath = Path.GetDirectoryName(objPath);
				objPath = "file://" + objPath;
				basepath = "file://" + basepath + Path.DirectorySeparatorChar;
			}
			else
			{
				basepath = ((objPath.IndexOf("/") == -1) ? "" : objPath.Substring(0, objPath.LastIndexOf("/") + 1));
			}
			StartCoroutine(Load(objPath));
		}
	}

	public IEnumerator Load(string path)
	{
		WWW loader2 = new WWW(path)
		{
			threadPriority = ThreadPriority.High
		};
		yield return loader2;
		if (loader2.error != null)
		{
			Debug.Log("File Not Found");
		}
		else
		{
			SetGeometryData(loader2.text);
		}
		if (buffer.Check(AutoResolveVLimit: true, DebugOut: false))
		{
			if (hasMaterials)
			{
				loader2 = new WWW(FileIO.ResolvePath(mtllib, basepath, wwwload: true));
				yield return loader2;
				if (loader2.error != null)
				{
					Debug.Log("failed to resolve material library path" + basepath + mtllib);
					string[] array = buffer.ReturnMaterialNames();
					this.materialData = new List<MaterialData>();
					new MaterialData();
					string[] array2 = array;
					foreach (string text in array2)
					{
						MaterialData materialData = new MaterialData();
						materialData.name = text;
						materialData.ShaderName = "Diffuse";
						materialData.diffuse = new Color(0.5f, 0.5f, 0.5f, 1f);
						this.materialData.Add(materialData);
					}
				}
				else
				{
					SetMaterialData(loader2.text);
					TMPTextures = new Texture2D[TextureList.Count];
					foreach (KeyValuePair<string, int> txt in TextureList)
					{
						switch (Path.GetExtension(txt.Key))
						{
						case ".jpg":
						case ".jpeg":
						case ".png":
						{
							WWW texloader = new WWW(FileIO.ResolvePath(txt.Key, basepath, wwwload: true));
							yield return texloader;
							if (texloader.error != null)
							{
								Debug.Log("failed to resolve texture path" + basepath + txt.Key);
							}
							else
							{
								TMPTextures[txt.Value] = texloader.texture;
							}
							break;
						}
						default:
							Debug.Log(txt.Key + ": image not correct file type please use jpeg or png");
							break;
						}
					}
					SolveMaterials();
				}
			}
			CheckScale();
			Build();
			FileIO.Loaded(success: true);
		}
		else
		{
			Debug.Log("Too many poly's");
			FileIO.Loaded(success: false);
		}
		UnityEngine.Object.Destroy(this);
	}

	private void pushscaleoffset(scaleoffset so, Vector3 v)
	{
		if (!so.initval)
		{
			so.minvxpos = v.x;
			so.maxvxpos = v.x;
			so.minvypos = v.y;
			so.maxvypos = v.y;
			so.minvzpos = v.z;
			so.maxvzpos = v.z;
			so.initval = true;
		}
		else
		{
			so.minvxpos = ((v.x < so.minvxpos) ? v.x : so.minvxpos);
			so.maxvxpos = ((v.x > so.maxvxpos) ? v.x : so.maxvxpos);
			so.minvypos = ((v.y < so.minvypos) ? v.y : so.minvypos);
			so.maxvypos = ((v.y > so.maxvypos) ? v.y : so.maxvypos);
			so.minvzpos = ((v.z < so.minvzpos) ? v.z : so.minvzpos);
			so.maxvzpos = ((v.z > so.maxvzpos) ? v.z : so.maxvzpos);
		}
	}

	private void SetGeometryData(string data)
	{
		data = data.Replace("\r\n", "\n");
		string[] array = data.Split("\n".ToCharArray());
		mainso = new scaleoffset();
		for (int i = 0; i < array.Length; i++)
		{
			string input = array[i];
			input = Regex.Replace(input, "# object", "o");
			if (input.IndexOf("#") != -1)
			{
				input = input.Substring(0, input.IndexOf("#"));
			}
			input = Regex.Replace(input, "\\s+", " ");
			input = input.Trim();
			string[] array2 = input.Split(" ".ToCharArray());
			switch (array2[0])
			{
			case "o":
				if (!EnforceSingleObj)
				{
					if (currentso != null)
					{
						currentso.vlast = vcount;
						objso.Add(currentso);
					}
					buffer.PushObject(array2[1].Trim());
					currentso = new scaleoffset();
					currentso.vfirst = vcount;
					currentso.name = array2[1].Trim();
				}
				break;
			case "g":
				buffer.PushGroup(array2[1].Trim());
				break;
			case "v":
				if (array2.Length >= 3)
				{
					Vector3 v = new Vector3(cf(array2[1]), cf(array2[2]), 0f - cf(array2[3]));
					buffer.PushVertex(v);
					vcount++;
					pushscaleoffset(mainso, v);
					if (currentso != null)
					{
						pushscaleoffset(currentso, v);
					}
				}
				break;
			case "vt":
				if (array2.Length >= 2)
				{
					buffer.PushUV(new Vector2(cf(array2[1]), cf(array2[2])));
				}
				break;
			case "vn":
				if (array2.Length >= 3)
				{
					buffer.PushNormal(new Vector3(cf(array2[1]), cf(array2[2]), 0f - cf(array2[3])));
				}
				break;
			case "f":
			{
				if (array2.Length < 4)
				{
					break;
				}
				if (array2.Length <= 5)
				{
					for (int j = 0; j < array2.Length - 3; j++)
					{
						FaceIndices f = default(FaceIndices);
						string[] array3 = array2[1].Trim().Split("/".ToCharArray());
						if (array3.Length != 0 && array3[0] != string.Empty)
						{
							f.vi = ci(array3[0]) - 1;
						}
						if (array3.Length > 1 && array3[1] != string.Empty)
						{
							f.vu = ci(array3[1]) - 1;
						}
						if (array3.Length > 2 && array3[2] != string.Empty)
						{
							f.vn = ci(array3[2]) - 1;
						}
						buffer.PushFace(f);
						for (int k = 0; k < 2; k++)
						{
							f = default(FaceIndices);
							int num = 3 - k + j;
							array3 = array2[num].Trim().Split("/".ToCharArray());
							if (array3.Length != 0 && array3[0] != string.Empty)
							{
								f.vi = ci(array3[0]) - 1;
							}
							if (array3.Length > 1 && array3[1] != string.Empty)
							{
								f.vu = ci(array3[1]) - 1;
							}
							if (array3.Length > 2 && array3[2] != string.Empty)
							{
								f.vn = ci(array3[2]) - 1;
							}
							buffer.PushFace(f);
						}
					}
					break;
				}
				TriPoly triPoly = new TriPoly();
				Vector3[] array4 = new Vector3[array2.Length - 1];
				for (int l = 1; l < array2.Length; l++)
				{
					string[] array5 = array2[l].Trim().Split("/".ToCharArray());
					if (array5.Length != 0 && array5[0] != string.Empty)
					{
						array4[l - 1] = buffer.vertices[ci(array5[0]) - 1];
					}
				}
				int[] array6 = triPoly.Patch(array4);
				if (array6.Length <= 2)
				{
					break;
				}
				for (int m = 0; m < array6.Length; m++)
				{
					FaceIndices f2 = default(FaceIndices);
					string[] array5 = array2[array6[m] + 1].Trim().Split("/".ToCharArray());
					if (array5.Length != 0 && array5[0] != string.Empty)
					{
						f2.vi = ci(array5[0]) - 1;
					}
					if (array5.Length > 1 && array5[1] != string.Empty)
					{
						f2.vu = ci(array5[1]) - 1;
					}
					if (array5.Length > 2 && array5[2] != string.Empty)
					{
						f2.vn = ci(array5[2]) - 1;
					}
					buffer.PushFace(f2);
				}
				break;
			}
			case "mtllib":
				mtllib = array2[1].Trim();
				break;
			case "usemtl":
				buffer.PushMaterialGroup(array2[1].Trim());
				break;
			}
		}
		if (currentso != null)
		{
			currentso.vlast = vcount;
			objso.Add(currentso);
		}
	}

	private void CalculateScaleOffset(scaleoffset so, bool above)
	{
		so.MasterOffsetx = 0f - (so.maxvxpos / 2f + so.minvxpos / 2f);
		so.MasterOffsety = 0f - (so.maxvypos / 2f + so.minvypos / 2f);
		so.MasterOffsetz = 0f - (so.maxvzpos / 2f + so.minvzpos / 2f);
		if (above)
		{
			so.MainOffset = new Vector3(so.MasterOffsetx, so.MasterOffsety + (so.maxvypos - so.minvypos) / 2f, so.MasterOffsetz);
		}
		else
		{
			so.MainOffset = new Vector3(so.MasterOffsetx, so.MasterOffsety, so.MasterOffsetz);
		}
		so.minvpos = ((so.minvypos < so.minvxpos) ? so.minvypos : so.minvxpos);
		so.minvpos = ((so.minvzpos < so.minvpos) ? so.minvzpos : so.minvpos);
		so.maxvpos = ((so.maxvypos > so.maxvxpos) ? so.maxvypos : so.maxvxpos);
		so.maxvpos = ((so.maxvzpos > so.maxvpos) ? so.maxvzpos : so.maxvpos);
		float num = 0f;
		num = ((0f - so.minvpos > so.maxvpos) ? (0f - so.minvpos) : so.maxvpos);
		if (num != 0f)
		{
			if (num > objmaxsize)
			{
				so.MasterScale = objmaxsize / num;
			}
			if (num < objminsize)
			{
				so.MasterScale = objminsize / num;
			}
		}
		else
		{
			so.MasterScale = 1f;
		}
	}

	private void CheckScale()
	{
		CalculateScaleOffset(mainso, above: true);
		if (!EnforceSingleObj && objso.Count > 1)
		{
			treatasoneobject = false;
			{
				foreach (scaleoffset item in objso)
				{
					CalculateScaleOffset(item, above: false);
					for (int i = item.vfirst; i < item.vlast; i++)
					{
						buffer.vertices[i] = (buffer.vertices[i] + item.MainOffset) * mainso.MasterScale;
					}
				}
				return;
			}
		}
		treatasoneobject = true;
		for (int j = 0; j < buffer.vertices.Count; j++)
		{
			buffer.vertices[j] = (buffer.vertices[j] + mainso.MainOffset) * mainso.MasterScale;
		}
	}

	private float cf(string v)
	{
		return Convert.ToSingle(v.Trim(), new CultureInfo("en-US"));
	}

	private int ci(string v)
	{
		return Convert.ToInt32(v.Trim(), new CultureInfo("en-US"));
	}

	private Color gc(string[] p)
	{
		return new Color(cf(p[1]), cf(p[2]), cf(p[3]));
	}

	private void SetMaterialData(string data)
	{
		data = data.Replace("\r\n", "\n");
		string[] array = data.Split("\n".ToCharArray());
		this.materialData = new List<MaterialData>();
		MaterialData materialData = new MaterialData();
		int num = 0;
		for (int i = 0; i < array.Length; i++)
		{
			string text = array[i];
			if (text.IndexOf("#") != -1)
			{
				text = text.Substring(0, text.IndexOf("#"));
			}
			text = Regex.Replace(text, "\\s+", " ");
			text = text.Trim();
			string[] array2 = text.Split(" ".ToCharArray());
			switch (array2[0])
			{
			case "newmtl":
				materialData = new MaterialData();
				materialData.name = array2[1].Trim();
				this.materialData.Add(materialData);
				break;
			case "Ka":
				materialData.ambient = gc(array2);
				break;
			case "Kd":
				materialData.diffuse = gc(array2);
				break;
			case "Ks":
				materialData.specular = gc(array2);
				break;
			case "Ke":
				materialData.emmisive = gc(array2);
				if (materialData.emmisive.grayscale > 0f)
				{
					materialData.emmision = true;
				}
				break;
			case "Ns":
				materialData.shininess = cf(array2[1]) / 1000f;
				break;
			case "d":
				materialData.alpha = 1f - cf(array2[1]);
				break;
			case "Tr":
				materialData.alpha = 1f - cf(array2[1]);
				break;
			case "map_Ka":
				materialData.ambientTexPath = array2[array2.Length - 1].Trim();
				if (!TextureList.ContainsKey(array2[array2.Length - 1].Trim()))
				{
					TextureList.Add(array2[array2.Length - 1].Trim(), num);
					num++;
				}
				break;
			case "map_Kd":
				materialData.diffuseTexPath = array2[array2.Length - 1].Trim();
				if (!TextureList.ContainsKey(array2[array2.Length - 1].Trim()))
				{
					TextureList.Add(array2[array2.Length - 1].Trim(), num);
					num++;
				}
				break;
			case "map_Ke":
				materialData.emmisiveTexPath = array2[array2.Length - 1].Trim();
				if (!TextureList.ContainsKey(array2[array2.Length - 1].Trim()))
				{
					TextureList.Add(array2[array2.Length - 1].Trim(), num);
					num++;
				}
				break;
			case "map_Ks":
				materialData.specularTexPath = array2[array2.Length - 1].Trim();
				if (!TextureList.ContainsKey(array2[array2.Length - 1].Trim()))
				{
					TextureList.Add(array2[array2.Length - 1].Trim(), num);
					num++;
				}
				break;
			case "map_d":
				materialData.alphaTexPath = array2[array2.Length - 1].Trim();
				if (!TextureList.ContainsKey(array2[array2.Length - 1].Trim()))
				{
					TextureList.Add(array2[array2.Length - 1].Trim(), num);
					num++;
				}
				break;
			case "map_tr":
				materialData.alphaTexPath = array2[array2.Length - 1].Trim();
				if (!TextureList.ContainsKey(array2[array2.Length - 1].Trim()))
				{
					TextureList.Add(array2[array2.Length - 1].Trim(), num);
					num++;
				}
				break;
			case "map_bump":
				materialData.bumpTexPath = array2[array2.Length - 1].Trim();
				if (!TextureList.ContainsKey(array2[array2.Length - 1].Trim()))
				{
					TextureList.Add(array2[array2.Length - 1].Trim(), num);
					num++;
				}
				break;
			case "illum":
				materialData.illumType = ci(array2[1]);
				break;
			}
		}
	}

	private void SolveMaterials()
	{
		foreach (MaterialData materialDatum in materialData)
		{
			Texture2D texture2D = new Texture2D(2, 2);
			bool flag = false;
			bool flag2 = false;
			materialDatum.ShaderName = "";
			if ((double)materialDatum.alpha == 1.0 && materialDatum.alphaTexPath == null)
			{
				if (materialDatum.emmision || materialDatum.emmisiveTexPath != null)
				{
					materialDatum.ShaderName += "Self-Illumin/";
					if (materialDatum.emmisiveTexPath != null && TextureList.ContainsKey(materialDatum.emmisiveTexPath))
					{
						texture2D = TMPTextures[TextureList[materialDatum.emmisiveTexPath]];
						flag2 = true;
						flag = true;
					}
				}
			}
			else if ((double)materialDatum.alpha == 1.0)
			{
				materialDatum.ShaderName += "Transparent/Cutout/";
				if (materialDatum.alphaTexPath != null && TextureList.ContainsKey(materialDatum.alphaTexPath))
				{
					texture2D = TMPTextures[TextureList[materialDatum.alphaTexPath]];
					flag = true;
				}
			}
			else
			{
				materialDatum.ShaderName += "Transparent/";
				if (materialDatum.alphaTexPath != null && TextureList.ContainsKey(materialDatum.alphaTexPath))
				{
					texture2D = TMPTextures[TextureList[materialDatum.alphaTexPath]];
					flag = true;
				}
			}
			if (materialDatum.bumpTexPath != null && TextureList.ContainsKey(materialDatum.bumpTexPath))
			{
				materialDatum.ShaderName += "Bumped ";
				materialDatum.BumpTexture = TMPTextures[TextureList[materialDatum.bumpTexPath]];
			}
			if ((double)materialDatum.shininess > 0.01 || materialDatum.specularTexPath != null)
			{
				materialDatum.ShaderName += "Specular";
				if (materialDatum.specularTexPath != null && !flag && TextureList.ContainsKey(materialDatum.specularTexPath))
				{
					texture2D = TMPTextures[TextureList[materialDatum.specularTexPath]];
					flag = true;
				}
			}
			else
			{
				materialDatum.ShaderName += "Diffuse";
			}
			if (materialDatum.diffuseTexPath != null && TextureList.ContainsKey(materialDatum.diffuseTexPath))
			{
				materialDatum.DiffTexture = TMPTextures[TextureList[materialDatum.diffuseTexPath]];
			}
			if (flag)
			{
				Texture2D texture2D2;
				if (materialDatum.DiffTexture != null)
				{
					texture2D = ScaleTexture(texture2D, materialDatum.DiffTexture.width, materialDatum.DiffTexture.height);
					texture2D2 = new Texture2D(materialDatum.DiffTexture.width, materialDatum.DiffTexture.height, TextureFormat.ARGB32, mipChain: false);
				}
				else
				{
					materialDatum.DiffTexture = new Texture2D(texture2D.width, texture2D.height, TextureFormat.ARGB32, mipChain: false);
					texture2D2 = new Texture2D(texture2D.width, texture2D.height, TextureFormat.ARGB32, mipChain: false);
				}
				Color[] pixels = texture2D.GetPixels(0);
				Color[] pixels2 = materialDatum.DiffTexture.GetPixels(0);
				Color[] pixels3 = texture2D2.GetPixels(0);
				int num = 0;
				if (!flag2)
				{
					if (texture2D.format == TextureFormat.ARGB32)
					{
						if (materialDatum.diffuseTexPath == null)
						{
							for (num = 0; num < pixels3.Length; num++)
							{
								pixels3[num] = new Color(0.5f, 0.5f, 0.5f, pixels[num].a);
							}
						}
						else
						{
							for (num = 0; num < pixels3.Length; num++)
							{
								pixels3[num] = new Color(pixels2[num].r, pixels2[num].g, pixels2[num].b, pixels[num].a);
							}
						}
					}
					else if (materialDatum.diffuseTexPath == null)
					{
						for (num = 0; num < pixels3.Length; num++)
						{
							pixels3[num] = new Color(0.5f, 0.5f, 0.5f, pixels[num].grayscale);
						}
					}
					else
					{
						for (num = 0; num < pixels3.Length; num++)
						{
							pixels3[num] = new Color(pixels2[num].r, pixels2[num].g, pixels2[num].b, pixels[num].grayscale);
						}
					}
					texture2D2.SetPixels(pixels3, 0);
					materialDatum.DiffTexture = texture2D2;
					materialDatum.DiffTexture.SetPixels(materialDatum.DiffTexture.GetPixels());
					materialDatum.DiffTexture.Apply(updateMipmaps: true);
					materialDatum.DiffTexture.name = materialDatum.name + "_MainTex";
					if (SaveToProject)
					{
						FileIO.SaveTextureToProject(materialDatum.DiffTexture, replace: false);
					}
					materialDatum.DiffTexture.Compress(highQuality: true);
				}
				else
				{
					if (texture2D.format == TextureFormat.ARGB32)
					{
						for (num = 0; num < pixels3.Length; num++)
						{
							pixels3[num] = new Color(1f, 1f, 1f, pixels[num].a);
						}
					}
					else
					{
						for (num = 0; num < pixels3.Length; num++)
						{
							pixels3[num] = new Color(1f, 1f, 1f, pixels[num].grayscale);
						}
					}
					texture2D2.SetPixels(pixels3, 0);
					materialDatum.EmmisiveTexture = texture2D2;
					materialDatum.EmmisiveTexture.SetPixels(materialDatum.EmmisiveTexture.GetPixels());
					materialDatum.EmmisiveTexture.Apply(updateMipmaps: true);
					materialDatum.EmmisiveTexture.name = materialDatum.name + "_Illum";
					if (SaveToProject)
					{
						FileIO.SaveTextureToProject(materialDatum.EmmisiveTexture, replace: false);
					}
					materialDatum.EmmisiveTexture.Compress(highQuality: true);
				}
			}
			else if (materialDatum.DiffTexture != null)
			{
				Texture2D texture2D2 = new Texture2D(materialDatum.DiffTexture.width, materialDatum.DiffTexture.height, materialDatum.DiffTexture.format, mipChain: false);
				Color[] pixels3 = materialDatum.DiffTexture.GetPixels(0);
				texture2D2.SetPixels(pixels3, 0);
				materialDatum.DiffTexture = texture2D2;
				materialDatum.DiffTexture.SetPixels(materialDatum.DiffTexture.GetPixels());
				materialDatum.DiffTexture.Apply(updateMipmaps: true);
				materialDatum.DiffTexture.name = materialDatum.name + "_MainTex";
				if (SaveToProject)
				{
					FileIO.SaveTextureToProject(materialDatum.DiffTexture, replace: false);
				}
				materialDatum.DiffTexture.Compress(highQuality: true);
			}
			if (materialDatum.BumpTexture != null)
			{
				materialDatum.BumpTexture = NormalMap(materialDatum.BumpTexture, BumpStrength);
				materialDatum.BumpTexture.filterMode = FilterMode.Trilinear;
				materialDatum.BumpTexture.name = materialDatum.name + "_BumpMap";
				if (SaveToProject)
				{
					FileIO.SaveTextureToProject(materialDatum.BumpTexture, replace: false);
				}
				materialDatum.BumpTexture.Compress(highQuality: true);
			}
		}
	}

	private Material GetMaterial(MaterialData md)
	{
		Material material = new Material(Shader.Find(md.ShaderName));
		material.SetColor("_Color", md.diffuse);
		if (md.ShaderName.Contains("Self-Illumin"))
		{
			if (md.EmmisiveTexture != null)
			{
				material.SetTexture("_Illum", md.EmmisiveTexture);
			}
			if (md.emmision)
			{
				material.SetColor("_Color", md.emmisive);
			}
		}
		if (md.ShaderName.Contains("Transparent"))
		{
			material.SetColor("_Color", new Color(md.diffuse.r, md.diffuse.g, md.diffuse.b, md.alpha));
		}
		if (md.ShaderName.Contains("Cutout"))
		{
			material.SetFloat("_Cutoff", 0.5f);
		}
		if (md.ShaderName.Contains("Specular"))
		{
			material.SetColor("_SpecColor", md.specular);
			material.SetFloat("_Shininess", md.shininess);
		}
		if (md.DiffTexture != null)
		{
			material.SetTexture("_MainTex", md.DiffTexture);
		}
		if (md.BumpTexture != null)
		{
			material.SetTexture("_BumpMap", md.BumpTexture);
		}
		return material;
	}

	private void Build()
	{
		Dictionary<string, Material> dictionary = new Dictionary<string, Material>();
		if (hasMaterials)
		{
			Material material = new Material(Shader.Find("Diffuse"));
			material.SetColor("_Color", new Color(0.5f, 0.5f, 0.5f, 1f));
			dictionary.Add("_spc_default", material);
			foreach (MaterialData materialDatum in materialData)
			{
				if (!dictionary.ContainsKey(materialDatum.name))
				{
					dictionary.Add(materialDatum.name, GetMaterial(materialDatum));
				}
			}
		}
		else
		{
			Material material2 = new Material(Shader.Find("Diffuse"));
			material2.SetColor("_Color", new Color(0.5f, 0.5f, 0.5f, 1f));
			dictionary.Add("_spc_default", material2);
		}
		GameObject[] array = new GameObject[buffer.numObjects];
		if (buffer.numObjects == 1)
		{
			if (!treatasoneobject)
			{
				GameObject gameObject = new GameObject();
				gameObject.transform.parent = base.gameObject.transform;
				gameObject.AddComponent(typeof(MeshFilter));
				gameObject.AddComponent(typeof(MeshRenderer));
				gameObject.name = "_spc_rename_";
				array[0] = gameObject;
			}
			else
			{
				base.gameObject.AddComponent(typeof(MeshFilter));
				base.gameObject.AddComponent(typeof(MeshRenderer));
				array[0] = base.gameObject;
			}
		}
		else if (buffer.numObjects > 1)
		{
			for (int i = 0; i < buffer.numObjects; i++)
			{
				GameObject gameObject2 = new GameObject();
				gameObject2.transform.parent = base.gameObject.transform;
				gameObject2.AddComponent(typeof(MeshFilter));
				gameObject2.AddComponent(typeof(MeshRenderer));
				array[i] = gameObject2;
			}
		}
		buffer.PopulateMeshes(array, dictionary);
		if (MakeCollider)
		{
			MeshOp.AddReducedMeshColliders(array, "progmesh");
		}
		if (SaveToProject)
		{
			FileIO.SaveMeshesToProject(array, replace: false);
		}
		if (!treatasoneobject)
		{
			GameObject[] array2 = array;
			foreach (GameObject go in array2)
			{
				scaleoffset scaleoffset = objso.Find((scaleoffset item) => item.name == go.name);
				if (scaleoffset != null)
				{
					go.transform.localPosition = (Vector3.zero - scaleoffset.MainOffset) * mainso.MasterScale;
				}
			}
		}
		FileIO.LastLoaded = array;
	}

	private Texture2D NormalMap(Texture2D source, float strength)
	{
		strength = Mathf.Clamp(strength, 0f, 10f);
		Texture2D texture2D = new Texture2D(source.width, source.height, TextureFormat.ARGB32, mipChain: true);
		for (int i = 0; i < texture2D.height; i++)
		{
			for (int j = 0; j < texture2D.width; j++)
			{
				float num = source.GetPixel(j - 1, i).grayscale * strength;
				float num2 = source.GetPixel(j + 1, i).grayscale * strength;
				float num3 = source.GetPixel(j, i - 1).grayscale * strength;
				float num4 = source.GetPixel(j, i + 1).grayscale * strength;
				float r = (num - num2 + 1f) * 0.5f;
				float num5 = (num3 - num4 + 1f) * 0.5f;
				texture2D.SetPixel(j, i, new Color(r, num5, 1f, num5));
			}
		}
		texture2D.Apply();
		return texture2D;
	}

	private Texture2D ScaleTexture(Texture2D source, int targetWidth, int targetHeight)
	{
		Texture2D texture2D = new Texture2D(targetWidth, targetHeight, source.format, mipChain: true);
		Color[] pixels = texture2D.GetPixels(0);
		float num = 1f / (float)source.width * ((float)source.width / (float)targetWidth);
		float num2 = 1f / (float)source.height * ((float)source.height / (float)targetHeight);
		for (int i = 0; i < pixels.Length; i++)
		{
			pixels[i] = source.GetPixelBilinear(num * ((float)i % (float)targetWidth), num2 * Mathf.Floor(i / targetWidth));
		}
		texture2D.SetPixels(pixels, 0);
		texture2D.Apply();
		return texture2D;
	}
}
