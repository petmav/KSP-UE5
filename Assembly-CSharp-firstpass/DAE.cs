using System;
using System.Collections;
using System.Collections.Generic;
using System.Globalization;
using System.IO;
using System.Text;
using System.Text.RegularExpressions;
using System.Xml;
using UnityEngine;

public class DAE : MonoBehaviour
{
	private class SceneGeo
	{
		public string ParentNode;

		public string GID;

		public bool isInstance;

		public Matrix4x4 Gm;

		public Quaternion Totalrot;

		public List<GMaterialID> GMaterial;

		public SceneGeo()
		{
			GMaterial = new List<GMaterialID>();
		}
	}

	private class GMaterialID
	{
		public string id;

		public string symbol;
	}

	private class SceneTransform
	{
		public string NodeName;

		public Vector3 pos;

		public Quaternion rot;

		public Vector3 scl;

		public bool hasgeometry;

		public string ParentNode;
	}

	private class DataID
	{
		public string ID;

		public string Semantic;
	}

	private class DAETextureID
	{
		public string id;

		public string path;
	}

	private class MaterialData
	{
		public string ID;

		public string name;

		public Color ambient;

		public Color diffuse;

		public Color specular = new Color(0f, 0f, 0f, 1f);

		public Color emmisive;

		public Color alphacolor = new Color(1f, 1f, 1f, 1f);

		public float shininess = 2f;

		public float alpha = 1f;

		public string diffuseTexPath;

		public string emmisiveTexPath;

		public string specularTexPath;

		public string alphaTexPath;

		public Texture2D DiffTexture;

		public Texture2D EmmisiveTexture;

		public string ShaderName;
	}

	public string objPath;

	public bool MakeCollider;

	public bool SaveToProject;

	public float objmaxsize = 1000f;

	public float objminsize = 0.001f;

	public bool EnforceSingleObj;

	private bool initval;

	private float MasterScale = 1f;

	private float minvpos;

	private float maxvpos;

	private float minvxpos;

	private float maxvxpos;

	private float minvypos;

	private float maxvypos;

	private float minvzpos;

	private float maxvzpos;

	private float MasterOffsetx;

	private float MasterOffsety;

	private float MasterOffsetz;

	private Vector3 MainOffset;

	private string basepath;

	private GeometryBuffer buffer;

	private Texture2D[] TMPTextures;

	private XmlDocument xdoc;

	private int Voffset;

	private int Noffset;

	private int Uoffset;

	private int instanceVoffset;

	private int instanceNoffset;

	private int instanceUoffset;

	private int up_axis = 1;

	private bool normalvertexgrouped;

	private bool treatasoneobject;

	private bool firstobject = true;

	private bool hasmaterials;

	private string currentgname = "";

	private int stwithoutgeometry;

	private List<SceneGeo> sceneGeo = new List<SceneGeo>();

	private List<SceneTransform> sceneTransforms = new List<SceneTransform>();

	private List<DAETextureID> daeTextures;

	private Dictionary<string, int> TextureList = new Dictionary<string, int>();

	private List<MaterialData> materialData;

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
		WWW loader = new WWW(path)
		{
			threadPriority = ThreadPriority.High
		};
		yield return loader;
		if (loader.error != null)
		{
			Debug.Log("File Not Found");
		}
		else
		{
			ReadDAE(loader.text);
		}
		if (buffer.Check(AutoResolveVLimit: true, DebugOut: false))
		{
			if (hasmaterials && !FileIO.ignoreMaterials)
			{
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
			else
			{
				string[] array = buffer.ReturnMaterialNames();
				this.materialData = new List<MaterialData>();
				new MaterialData();
				string[] array2 = array;
				foreach (string iD in array2)
				{
					MaterialData materialData = new MaterialData();
					materialData.ID = iD;
					materialData.name = iD;
					materialData.ShaderName = "Diffuse";
					materialData.diffuse = new Color(0.5f, 0.5f, 0.5f, 1f);
					this.materialData.Add(materialData);
				}
				hasmaterials = true;
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

	private void GetUpAxis()
	{
		if ((XmlElement)xdoc.GetElementsByTagName("up_axis")[0] != null)
		{
			switch (((XmlElement)xdoc.GetElementsByTagName("up_axis")[0]).InnerXml)
			{
			case "X_UP":
				up_axis = 0;
				break;
			case "Y_UP":
				up_axis = 1;
				break;
			case "Z_UP":
				up_axis = 2;
				break;
			}
		}
		else
		{
			up_axis = 1;
		}
	}

	private void ReadDAE(string gdata)
	{
		buffer = new GeometryBuffer();
		xdoc = new XmlDocument();
		xdoc.LoadXml(gdata);
		GetUpAxis();
		GetDAEMaterials();
		if (GetSceneGeo())
		{
			foreach (XmlNode item in xdoc.GetElementsByTagName("geometry"))
			{
				currentgname = "";
				Voffset = ((buffer.vertices.Count > 0) ? buffer.vertices.Count : 0);
				Uoffset = ((buffer.uvs.Count > 0) ? buffer.uvs.Count : 0);
				Noffset = ((buffer.normals.Count > 0) ? buffer.normals.Count : 0);
				XmlElement xmlElement = item as XmlElement;
				string text = CanParseGeometry(xmlElement);
				if (text != "false")
				{
					currentgname = xmlElement.Attributes["id"].Value;
					bool flag = false;
					foreach (SceneGeo item2 in sceneGeo)
					{
						if (currentgname == item2.GID)
						{
							flag = true;
						}
					}
					if (flag)
					{
						if (GetGeometryData(xmlElement))
						{
							if (!GetFaceData(xmlElement, text))
							{
								Debug.Log("Geometry " + xmlElement.Attributes["id"].Value + "has no readable face data");
							}
						}
						else
						{
							Debug.Log("Geometry " + xmlElement.Attributes["id"].Value + " has no readable source data");
						}
					}
				}
			}
			return;
		}
		Debug.Log("Cannot get scene data matching geometry to materials");
	}

	private Vector3 UpPosConv(Vector3 v)
	{
		Vector3 result = default(Vector3);
		switch (up_axis)
		{
		case 0:
			result = new Vector3(0f - v.y, v.x, 0f - v.z);
			break;
		case 1:
			result = new Vector3(v.x, v.y, 0f - v.z);
			break;
		case 2:
			result = new Vector3(v.x, v.z, v.y);
			break;
		}
		return result;
	}

	private Vector3 UpScaleConv(Vector3 v)
	{
		Vector3 result = default(Vector3);
		switch (up_axis)
		{
		case 0:
			result = new Vector3(v.y, v.x, v.z);
			break;
		case 1:
			result = new Vector3(v.x, v.y, v.z);
			break;
		case 2:
			result = new Vector3(v.x, v.z, v.y);
			break;
		}
		return result;
	}

	private Vector3 UpRotDirConv(Vector3 v)
	{
		Vector3 result = default(Vector3);
		switch (up_axis)
		{
		case 0:
			result = new Vector3(v.y, 0f - v.x, v.z);
			break;
		case 1:
			result = new Vector3(0f - v.x, 0f - v.y, v.z);
			break;
		case 2:
			result = new Vector3(0f - v.x, 0f - v.z, 0f - v.y);
			break;
		}
		return result;
	}

	private bool GetSceneGeo()
	{
		bool flag = false;
		if ((XmlElement)xdoc.GetElementsByTagName("library_visual_scenes")[0] != null)
		{
			flag = true;
		}
		if (flag)
		{
			XmlElement obj = (XmlElement)((XmlElement)xdoc.GetElementsByTagName("library_visual_scenes")[0]).GetElementsByTagName("visual_scene")[0];
			XmlNode firstChild = obj.FirstChild;
			firstChild = obj.FirstChild;
			int num = 0;
			int num2 = 0;
			List<Vector3> list = new List<Vector3>();
			List<Vector3> list2 = new List<Vector3>();
			List<Quaternion> list3 = new List<Quaternion>();
			List<XmlNode> list4 = new List<XmlNode>();
			list4.Add(firstChild);
			while (num != -1 && firstChild is XmlElement)
			{
				XmlElement xmlElement = firstChild as XmlElement;
				if (firstChild.Name == "node")
				{
					SceneTransform sceneTransform = new SceneTransform();
					if (xmlElement.HasAttribute("id"))
					{
						sceneTransform.NodeName = xmlElement.Attributes["id"].Value;
					}
					else if (xmlElement.HasAttribute("name"))
					{
						sceneTransform.NodeName = xmlElement.Attributes["name"].Value;
					}
					else
					{
						sceneTransform.NodeName = "_spc_offset_" + num2;
						xmlElement.SetAttribute("id", sceneTransform.NodeName);
						num2++;
					}
					Vector3 vector = new Vector3(0f, 0f, 0f);
					Vector3 vector2 = new Vector3(1f, 1f, 1f);
					Quaternion identity = Quaternion.identity;
					if (xmlElement["translate"] != null || xmlElement["scale"] != null || xmlElement["rotate"] != null)
					{
						if (xmlElement["translate"] != null)
						{
							string[] array = RemoveWS(xmlElement["translate"].InnerXml).Split(" ".ToCharArray());
							vector = new Vector3(cf(array[0]), cf(array[1]), cf(array[2]));
							vector = UpPosConv(vector);
						}
						if (xmlElement["scale"] != null)
						{
							string[] array2 = RemoveWS(xmlElement["scale"].InnerXml).Split(" ".ToCharArray());
							vector2 = new Vector3(cf(array2[0]), cf(array2[1]), cf(array2[2]));
							vector2 = UpScaleConv(vector2);
						}
						if (xmlElement["rotate"] != null)
						{
							XmlNode xmlNode = xmlElement.FirstChild;
							while (xmlNode != null && xmlNode is XmlElement)
							{
								if (xmlNode.Name == "rotate")
								{
									string[] array3 = RemoveWS(xmlNode.InnerXml).Split(" ".ToCharArray());
									Vector3 v = new Vector3(cf(array3[0]), cf(array3[1]), cf(array3[2]));
									v = UpRotDirConv(v);
									identity *= Quaternion.AngleAxis(cf(array3[3]), v);
								}
								xmlNode = xmlNode.NextSibling;
							}
						}
					}
					sceneTransform.pos = vector;
					sceneTransform.scl = vector2;
					sceneTransform.rot = identity;
					list.Add(vector);
					list2.Add(vector2);
					list3.Add(identity);
					if (firstChild.ParentNode != null)
					{
						XmlElement xmlElement2 = firstChild.ParentNode as XmlElement;
						if (xmlElement2.Name == "node")
						{
							if (xmlElement2.HasAttribute("id"))
							{
								sceneTransform.ParentNode = xmlElement2.Attributes["id"].Value;
							}
							else if (xmlElement2.HasAttribute("name"))
							{
								sceneTransform.ParentNode = xmlElement2.Attributes["name"].Value;
							}
						}
					}
					if (xmlElement["instance_geometry"] != null)
					{
						sceneTransform.hasgeometry = true;
						XmlNode xmlNode2 = xmlElement.FirstChild;
						while (xmlNode2 != null && xmlNode2 is XmlElement)
						{
							if (xmlNode2.Name == "instance_geometry")
							{
								SceneGeo sg = new SceneGeo();
								sg.GID = xmlNode2.Attributes["url"].Value.Replace("#", "");
								if (sceneGeo.Find((SceneGeo item) => item.GID == sg.GID) != null)
								{
									sg.isInstance = true;
								}
								sg.ParentNode = sceneTransform.NodeName;
								foreach (XmlNode item in (xmlNode2 as XmlElement).GetElementsByTagName("instance_material"))
								{
									XmlElement xmlElement3 = item as XmlElement;
									GMaterialID gMaterialID = new GMaterialID();
									gMaterialID.id = xmlElement3.Attributes["target"].Value.Replace("#", "");
									gMaterialID.symbol = xmlElement3.Attributes["symbol"].Value;
									sg.GMaterial.Add(gMaterialID);
								}
								Vector3 pos = new Vector3(0f, 0f, 0f);
								Vector3 vector3 = new Vector3(1f, 1f, 1f);
								Quaternion identity2 = Quaternion.identity;
								Matrix4x4 matrix4x = Matrix4x4.TRS(pos, identity2, vector3);
								for (int i = 0; i < list.Count; i++)
								{
									Vector3 pos2 = matrix4x.MultiplyPoint3x4(list[i]);
									vector3 = Vector3.Scale(vector3, list2[i]);
									identity2 *= list3[i];
									matrix4x = Matrix4x4.TRS(pos2, identity2, vector3);
								}
								Matrix4x4 gm = matrix4x;
								sg.Totalrot = identity2;
								sg.Gm = gm;
								sceneGeo.Add(sg);
							}
							xmlNode2 = xmlNode2.NextSibling;
						}
					}
					else
					{
						sceneTransform.hasgeometry = false;
					}
					if (xmlElement["node"] != null || sceneTransform.hasgeometry)
					{
						sceneTransforms.Add(sceneTransform);
					}
				}
				if (xmlElement["node"] != null)
				{
					firstChild = xmlElement["node"];
					list4.Add(firstChild);
					num++;
					continue;
				}
				bool flag2 = false;
				while (!flag2)
				{
					firstChild = firstChild.NextSibling;
					if (firstChild == null)
					{
						list4.RemoveAt(list4.Count - 1);
						if (list.Count != 0)
						{
							list.RemoveAt(list.Count - 1);
							list2.RemoveAt(list2.Count - 1);
							list3.RemoveAt(list3.Count - 1);
						}
						if (list4.Count != 0)
						{
							firstChild = list4[list4.Count - 1];
						}
						num--;
						if (num == -1)
						{
							flag2 = true;
						}
					}
					else
					{
						list4.RemoveAt(list4.Count - 1);
						if (list.Count != 0)
						{
							list.RemoveAt(list.Count - 1);
							list2.RemoveAt(list2.Count - 1);
							list3.RemoveAt(list3.Count - 1);
						}
						list4.Add(firstChild);
						flag2 = true;
					}
				}
			}
		}
		treatasoneobject = false;
		int num3 = 0;
		stwithoutgeometry = 0;
		foreach (SceneTransform sceneTransform2 in sceneTransforms)
		{
			if (!sceneTransform2.NodeName.Contains("_spc_offset_"))
			{
				num3++;
			}
			if (!sceneTransform2.hasgeometry)
			{
				stwithoutgeometry++;
			}
		}
		if (num3 == 1)
		{
			treatasoneobject = true;
		}
		if (EnforceSingleObj)
		{
			treatasoneobject = true;
		}
		return flag;
	}

	private string CanParseGeometry(XmlElement georoot)
	{
		string text = "false";
		if ((XmlElement)georoot.GetElementsByTagName("mesh")[0] != null)
		{
			if ((XmlElement)georoot.GetElementsByTagName("lines")[0] == null)
			{
				if ((XmlElement)georoot.GetElementsByTagName("triangles")[0] != null)
				{
					return "triangles";
				}
				if ((XmlElement)georoot.GetElementsByTagName("polygons")[0] != null)
				{
					return "polygons";
				}
				if ((XmlElement)georoot.GetElementsByTagName("polylist")[0] != null)
				{
					return "polylist";
				}
				return "false";
			}
			return "false";
		}
		return "false";
	}

	private void pushtooffsetscale(Vector3 v)
	{
		if (!initval)
		{
			minvxpos = v.x;
			maxvxpos = v.x;
			minvypos = v.y;
			maxvypos = v.y;
			minvzpos = v.z;
			maxvzpos = v.z;
			initval = true;
		}
		else
		{
			minvxpos = ((v.x < minvxpos) ? v.x : minvxpos);
			maxvxpos = ((v.x > maxvxpos) ? v.x : maxvxpos);
			minvypos = ((v.y < minvypos) ? v.y : minvypos);
			maxvypos = ((v.y > maxvypos) ? v.y : maxvypos);
			minvzpos = ((v.z < minvzpos) ? v.z : minvzpos);
			maxvzpos = ((v.z > maxvzpos) ? v.z : maxvzpos);
		}
	}

	private bool GetGeometryData(XmlElement georoot)
	{
		bool result = false;
		List<DataID> list = new List<DataID>();
		foreach (XmlNode item in georoot.GetElementsByTagName("input"))
		{
			XmlElement xmlElement = item as XmlElement;
			DataID dataID = new DataID();
			dataID.ID = xmlElement.Attributes["source"].Value.Replace("#", "");
			dataID.Semantic = xmlElement.Attributes["semantic"].Value;
			if (dataID.Semantic == "NORMAL" && xmlElement.ParentNode.Name == "vertices")
			{
				normalvertexgrouped = true;
			}
			list.Add(dataID);
		}
		List<SceneGeo> list2 = new List<SceneGeo>();
		SceneGeo sceneGeo = new SceneGeo();
		foreach (SceneGeo item2 in this.sceneGeo)
		{
			if (item2.GID == currentgname)
			{
				SceneGeo sceneGeo2 = new SceneGeo();
				sceneGeo2 = item2;
				list2.Add(sceneGeo2);
			}
		}
		int num = 1;
		if (treatasoneobject)
		{
			num = list2.Count;
		}
		for (int i = 0; i < num; i++)
		{
			sceneGeo = list2[i];
			foreach (XmlNode item3 in georoot.GetElementsByTagName("source"))
			{
				XmlElement xmlElement2 = item3 as XmlElement;
				foreach (DataID item4 in list)
				{
					if (!(item3.Attributes["id"].Value == item4.ID))
					{
						continue;
					}
					XmlElement xmlElement3 = (XmlElement)xmlElement2.GetElementsByTagName("float_array")[0];
					XmlElement obj = (XmlElement)xmlElement2.GetElementsByTagName("accessor")[0];
					int num2 = int.Parse(obj.Attributes["stride"].Value);
					int num3 = int.Parse(obj.Attributes["count"].Value);
					int num4 = int.Parse(xmlElement3.Attributes["count"].Value);
					string[] array = RemoveWS(xmlElement3.InnerXml).Split(" ".ToCharArray());
					if (array.Length == num4)
					{
						Vector3 vector = default(Vector3);
						for (int j = 0; j < array.Length; j += num2)
						{
							if (item4.Semantic == "POSITION")
							{
								instanceVoffset = num3;
								Vector3 v = new Vector3(cf(array[j]), cf(array[j + 1]), cf(array[j + 2]));
								v = UpPosConv(v);
								if (treatasoneobject)
								{
									v = sceneGeo.Gm.MultiplyPoint3x4(v);
									buffer.PushVertex(v);
									pushtooffsetscale(v);
								}
								else
								{
									buffer.PushVertex(v);
									foreach (SceneGeo item5 in list2)
									{
										vector = item5.Gm.MultiplyPoint3x4(v);
										pushtooffsetscale(vector);
									}
								}
								result = true;
							}
							if (item4.Semantic == "NORMAL")
							{
								instanceNoffset = num3;
								Vector3 v = new Vector3(cf(array[j]), cf(array[j + 1]), cf(array[j + 2]));
								v = UpPosConv(v);
								if (treatasoneobject)
								{
									v = sceneGeo.Totalrot * v;
								}
								buffer.PushNormal(v);
								result = true;
							}
							if (item4.Semantic == "TEXCOORD")
							{
								instanceUoffset = num3;
								buffer.PushUV(new Vector2(cf(array[j]), cf(array[j + 1])));
								result = true;
							}
						}
					}
					else
					{
						Debug.Log("In Geometry " + georoot.Attributes["id"].Value + " have problem parsing xml @ " + item4.Semantic);
					}
				}
			}
		}
		return result;
	}

	private string[] FormatFaceData(XmlElement FaceSection, int IndiceCount, int FaceCount)
	{
		string[] array = new string[FaceCount];
		XmlNodeList elementsByTagName = FaceSection.GetElementsByTagName("p");
		if (elementsByTagName.Count > 0)
		{
			int num = 0;
			if (elementsByTagName.Count < 2)
			{
				XmlElement xmlElement = (XmlElement)FaceSection.GetElementsByTagName("vcount")[0];
				XmlElement xmlElement2 = (XmlElement)FaceSection.GetElementsByTagName("p")[0];
				if (xmlElement != null)
				{
					string[] array2 = xmlElement.InnerXml.Split(" ".ToCharArray());
					string[] array3 = RemoveWS(xmlElement2.InnerXml).Split(" "[0]);
					int num2 = 0;
					int num3 = 0;
					while (num2 < array3.Length)
					{
						StringBuilder stringBuilder = new StringBuilder();
						for (int i = 0; i < int.Parse(array2[num3]); i++)
						{
							for (int j = 0; j < IndiceCount; j++)
							{
								stringBuilder.Append(array3[num2 + i * IndiceCount + j]).Append("/");
							}
							stringBuilder.Remove(stringBuilder.Length - 1, 1);
							stringBuilder.Append(" ");
						}
						array[num] = stringBuilder.ToString();
						num++;
						num2 += int.Parse(array2[num3]) * IndiceCount;
						num3++;
					}
				}
				else
				{
					string[] array4 = RemoveWS(xmlElement2.InnerXml).Split(" "[0]);
					for (int k = 0; k < array4.Length; k += 3 * IndiceCount)
					{
						StringBuilder stringBuilder2 = new StringBuilder();
						for (int l = 0; l < 3; l++)
						{
							for (int m = 0; m < IndiceCount; m++)
							{
								stringBuilder2.Append(array4[k + l * IndiceCount + m]).Append("/");
							}
							stringBuilder2.Remove(stringBuilder2.Length - 1, 1);
							stringBuilder2.Append(" ");
						}
						array[num] = stringBuilder2.ToString();
						num++;
					}
				}
			}
			else
			{
				foreach (XmlNode item in elementsByTagName)
				{
					string[] array5 = RemoveWS(item.InnerXml).Split(" "[0]);
					StringBuilder stringBuilder3 = new StringBuilder();
					for (int n = 0; n < array5.Length; n += IndiceCount)
					{
						for (int num4 = 0; num4 < IndiceCount; num4++)
						{
							stringBuilder3.Append(array5[n + num4]).Append("/");
						}
						stringBuilder3.Remove(stringBuilder3.Length - 1, 1);
						stringBuilder3.Append(" ");
					}
					array[num] = stringBuilder3.ToString();
					num++;
				}
			}
		}
		else
		{
			Debug.Log("Cannot Find Any Face Data For this Geometry");
		}
		return array;
	}

	private bool GetFaceData(XmlElement georoot, string gtype)
	{
		bool result = false;
		if (firstobject)
		{
			buffer.PushObject(currentgname);
			if (treatasoneobject)
			{
				firstobject = false;
			}
		}
		List<SceneGeo> list = new List<SceneGeo>();
		SceneGeo sceneGeo = new SceneGeo();
		foreach (SceneGeo item in this.sceneGeo)
		{
			if (item.GID == currentgname)
			{
				SceneGeo sceneGeo2 = new SceneGeo();
				sceneGeo2 = item;
				list.Add(sceneGeo2);
			}
		}
		int num = 1;
		if (treatasoneobject)
		{
			num = list.Count;
		}
		for (int i = 0; i < num; i++)
		{
			sceneGeo = list[i];
			int num2 = instanceVoffset * i;
			int num3 = instanceNoffset * i;
			int num4 = instanceUoffset * i;
			foreach (XmlNode item2 in georoot.GetElementsByTagName(gtype))
			{
				XmlElement xmlElement = item2 as XmlElement;
				if (int.Parse(xmlElement.Attributes["count"].Value) == 0)
				{
					continue;
				}
				string text = "_spc_default";
				bool flag = false;
				foreach (GMaterialID item3 in sceneGeo.GMaterial)
				{
					if (item3.symbol == xmlElement.Attributes["material"].Value)
					{
						text = item3.id;
						flag = true;
					}
				}
				if (!flag)
				{
					text = currentgname;
				}
				buffer.PushMaterialGroup(text);
				XmlNodeList elementsByTagName = xmlElement.GetElementsByTagName("input");
				string[] array = new string[elementsByTagName.Count];
				for (int j = 0; j < elementsByTagName.Count; j++)
				{
					array[j] = elementsByTagName[j].Attributes["semantic"].Value;
				}
				string[] array2 = FormatFaceData(xmlElement, array.Length, int.Parse(xmlElement.Attributes["count"].Value));
				for (int k = 0; k < array2.Length; k++)
				{
					string[] array3 = RemoveWS(array2[k]).Split(" ".ToCharArray());
					if (array3.Length <= 4)
					{
						for (int l = 0; l < array3.Length - 2; l++)
						{
							FaceIndices f = default(FaceIndices);
							string[] array4 = array3[0].Trim().Split("/".ToCharArray());
							for (int m = 0; m < array.Length; m++)
							{
								if (array[m] == "VERTEX")
								{
									f.vi = ci(array4[m]) + Voffset + num2;
									if (normalvertexgrouped)
									{
										f.vn = ci(array4[m]) + Noffset + num3;
									}
								}
								if (array[m] == "NORMAL")
								{
									f.vn = ci(array4[m]) + Noffset + num3;
								}
								if (array[m] == "TEXCOORD")
								{
									f.vu = ci(array4[m]) + Uoffset + num4;
								}
							}
							buffer.PushFace(f);
							for (int n = 0; n < 2; n++)
							{
								f = default(FaceIndices);
								int num5 = 2 - n + l;
								array4 = array3[num5].Trim().Split("/".ToCharArray());
								for (int num6 = 0; num6 < array.Length; num6++)
								{
									if (array[num6] == "VERTEX")
									{
										f.vi = ci(array4[num6]) + Voffset + num2;
										if (normalvertexgrouped)
										{
											f.vn = ci(array4[num6]) + Noffset + num3;
										}
									}
									if (array[num6] == "NORMAL")
									{
										f.vn = ci(array4[num6]) + Noffset + num3;
									}
									if (array[num6] == "TEXCOORD")
									{
										f.vu = ci(array4[num6]) + Uoffset + num4;
									}
								}
								buffer.PushFace(f);
							}
						}
						continue;
					}
					TriPoly triPoly = new TriPoly();
					Vector3[] array5 = new Vector3[array3.Length];
					for (int num7 = 0; num7 < array3.Length; num7++)
					{
						string[] array4 = array3[num7].Trim().Split("/".ToCharArray());
						for (int num8 = 0; num8 < array.Length; num8++)
						{
							if (array[num8] == "VERTEX")
							{
								array5[num7] = buffer.vertices[ci(array4[num8]) + Voffset + num2];
							}
						}
					}
					int[] array6 = triPoly.Patch(array5);
					if (array6.Length <= 2)
					{
						continue;
					}
					for (int num9 = 0; num9 < array6.Length; num9++)
					{
						FaceIndices f2 = default(FaceIndices);
						string[] array4 = array3[array6[num9]].Trim().Split("/".ToCharArray());
						for (int num10 = 0; num10 < array.Length; num10++)
						{
							if (array[num10] == "VERTEX")
							{
								f2.vi = ci(array4[num10]) + Voffset + num2;
								if (normalvertexgrouped)
								{
									f2.vn = ci(array4[num10]) + Noffset + num3;
								}
							}
							if (array[num10] == "NORMAL")
							{
								f2.vn = ci(array4[num10]) + Noffset + num3;
							}
							if (array[num10] == "TEXCOORD")
							{
								f2.vu = ci(array4[num10]) + Uoffset + num4;
							}
						}
						buffer.PushFace(f2);
					}
				}
				result = true;
			}
		}
		return result;
	}

	private void CheckScale()
	{
		MasterOffsetx = 0f - (maxvxpos / 2f + minvxpos / 2f);
		MasterOffsety = 0f - (maxvypos / 2f + minvypos / 2f);
		MasterOffsetz = 0f - (maxvzpos / 2f + minvzpos / 2f);
		MainOffset = new Vector3(MasterOffsetx, MasterOffsety + (maxvypos - minvypos) / 2f, MasterOffsetz);
		minvpos = ((minvypos < minvxpos) ? minvypos : minvxpos);
		minvpos = ((minvzpos < minvpos) ? minvzpos : minvpos);
		maxvpos = ((maxvypos > maxvxpos) ? maxvypos : maxvxpos);
		maxvpos = ((maxvzpos > maxvpos) ? maxvzpos : maxvpos);
		float num = 0f;
		num = ((0f - minvpos > maxvpos) ? (0f - minvpos) : maxvpos);
		if (num == 0f)
		{
			return;
		}
		if (num > objmaxsize)
		{
			MasterScale = objmaxsize / num;
		}
		if (num < objminsize)
		{
			MasterScale = objminsize / num;
		}
		if (treatasoneobject)
		{
			for (int i = 0; i < buffer.vertices.Count; i++)
			{
				buffer.vertices[i] = (buffer.vertices[i] + MainOffset) * MasterScale;
			}
		}
		else
		{
			for (int j = 0; j < buffer.vertices.Count; j++)
			{
				buffer.vertices[j] = buffer.vertices[j] * MasterScale;
			}
		}
	}

	private void GetDAEMaterials()
	{
		hasmaterials = true;
		int num = 0;
		daeTextures = new List<DAETextureID>();
		foreach (XmlNode item in xdoc.GetElementsByTagName("image"))
		{
			XmlElement xmlElement = item as XmlElement;
			DAETextureID dAETextureID = new DAETextureID();
			dAETextureID.id = xmlElement.Attributes["id"].Value;
			XmlElement xmlElement2 = (XmlElement)xmlElement.GetElementsByTagName("init_from")[0];
			dAETextureID.path = xmlElement2.InnerXml;
			daeTextures.Add(dAETextureID);
		}
		this.materialData = new List<MaterialData>();
		List<string> list = new List<string> { "emission", "ambient", "specular", "diffuse", "shininess", "transparent", "transparency" };
		XmlElement xmlElement3 = (XmlElement)xdoc.GetElementsByTagName("library_materials")[0];
		XmlElement xmlElement4 = (XmlElement)xdoc.GetElementsByTagName("library_effects")[0];
		if ((XmlElement)xdoc.GetElementsByTagName("library_materials")[0] != null)
		{
			foreach (XmlNode item2 in xmlElement3.GetElementsByTagName("material"))
			{
				XmlElement xmlElement5 = item2 as XmlElement;
				MaterialData materialData = new MaterialData();
				materialData.ID = xmlElement5.Attributes["id"].Value;
				if (xmlElement5.HasAttribute("name"))
				{
					materialData.name = xmlElement5.Attributes["name"].Value;
				}
				string text = ((XmlElement)xmlElement5.GetElementsByTagName("instance_effect")[0]).Attributes["url"].Value.Replace("#", "");
				foreach (XmlNode item3 in xmlElement4.GetElementsByTagName("effect"))
				{
					XmlElement xmlElement6 = item3 as XmlElement;
					if (xmlElement6.Attributes["id"].Value == text)
					{
						foreach (string item4 in list)
						{
							XmlElement xmlElement7 = (XmlElement)xmlElement6.GetElementsByTagName(item4)[0];
							if (xmlElement7 != null)
							{
								XmlElement xmlElement8 = (XmlElement)xmlElement7.GetElementsByTagName("color")[0];
								XmlElement xmlElement9 = (XmlElement)xmlElement7.GetElementsByTagName("float")[0];
								XmlElement xmlElement10 = (XmlElement)xmlElement7.GetElementsByTagName("texture")[0];
								switch (item4)
								{
								case "diffuse":
									if (xmlElement8 != null)
									{
										materialData.diffuse = DAEGetColor(xmlElement8);
									}
									if (xmlElement10 != null)
									{
										materialData.diffuseTexPath = GetValidTexturePath(xmlElement10.Attributes["texture"].Value, xmlElement6);
										if (materialData.diffuseTexPath != null && !TextureList.ContainsKey(materialData.diffuseTexPath))
										{
											TextureList.Add(materialData.diffuseTexPath, num);
											num++;
										}
									}
									break;
								case "emission":
									if (xmlElement8 != null)
									{
										materialData.emmisive = DAEGetColor(xmlElement8);
									}
									if (xmlElement10 != null)
									{
										materialData.emmisiveTexPath = GetValidTexturePath(xmlElement10.Attributes["texture"].Value, xmlElement6);
										if (materialData.emmisiveTexPath != null && !TextureList.ContainsKey(materialData.emmisiveTexPath))
										{
											TextureList.Add(materialData.emmisiveTexPath, num);
											num++;
										}
									}
									break;
								case "ambient":
									if (xmlElement8 != null)
									{
										materialData.ambient = DAEGetColor(xmlElement8);
									}
									break;
								case "specular":
									if (xmlElement8 != null)
									{
										materialData.specular = DAEGetColor(xmlElement8);
									}
									if (xmlElement10 != null)
									{
										materialData.specularTexPath = GetValidTexturePath(xmlElement10.Attributes["texture"].Value, xmlElement6);
										if (materialData.specularTexPath != null && !TextureList.ContainsKey(materialData.specularTexPath))
										{
											TextureList.Add(materialData.specularTexPath, num);
											num++;
										}
									}
									break;
								case "shininess":
									if (xmlElement9 != null)
									{
										materialData.shininess = cf(xmlElement9.InnerXml);
									}
									break;
								case "transparent":
									if (xmlElement8 != null)
									{
										materialData.alphacolor = DAEGetColor(xmlElement8);
									}
									if (xmlElement10 != null)
									{
										materialData.alphaTexPath = GetValidTexturePath(xmlElement10.Attributes["texture"].Value, xmlElement6);
										if (materialData.alphaTexPath != null && !TextureList.ContainsKey(materialData.alphaTexPath))
										{
											TextureList.Add(materialData.alphaTexPath, num);
											num++;
										}
									}
									break;
								case "transparency":
									if (xmlElement9 != null)
									{
										materialData.alpha = cf(xmlElement9.InnerXml);
									}
									break;
								}
							}
						}
					}
				}
				this.materialData.Add(materialData);
			}
			return;
		}
		hasmaterials = false;
	}

	private Color DAEGetColor(XmlElement elem)
	{
		string[] array = RemoveWS(elem.InnerXml).Split(" ".ToCharArray());
		return new Color(cf(array[0]), cf(array[1]), cf(array[2]), cf(array[3]));
	}

	private string GetValidTexturePath(string ID, XmlElement fx)
	{
		string text = "";
		foreach (DAETextureID daeTexture in daeTextures)
		{
			if (daeTexture.id == ID)
			{
				text = daeTexture.path;
			}
		}
		if (text == "")
		{
			bool flag = false;
			XmlNodeList elementsByTagName = fx.GetElementsByTagName("newparam");
			for (int i = 0; i < elementsByTagName.Count; i++)
			{
				foreach (XmlNode item in elementsByTagName)
				{
					XmlElement xmlElement = item as XmlElement;
					if (xmlElement.HasAttribute("sid"))
					{
						if (xmlElement.Attributes["sid"].Value == ID)
						{
							if ((XmlElement)xmlElement.GetElementsByTagName("source")[0] != null)
							{
								ID = ((XmlElement)xmlElement.GetElementsByTagName("source")[0]).InnerXml;
							}
							else if ((XmlElement)xmlElement.GetElementsByTagName("init_from")[0] != null)
							{
								ID = ((XmlElement)xmlElement.GetElementsByTagName("init_from")[0]).InnerXml;
								flag = true;
							}
						}
					}
					else
					{
						Debug.Log("Problems accessing texture path: - no SID");
					}
				}
				if (flag)
				{
					break;
				}
			}
			if (flag)
			{
				foreach (DAETextureID daeTexture2 in daeTextures)
				{
					if (daeTexture2.id == ID)
					{
						text = daeTexture2.path;
					}
				}
			}
		}
		return text;
	}

	private void SolveMaterials()
	{
		foreach (MaterialData materialDatum in materialData)
		{
			Texture2D texture2D = new Texture2D(2, 2);
			if (materialDatum.diffuse.grayscale == 0f)
			{
				if (materialDatum.ambient.grayscale != 0f)
				{
					materialDatum.diffuse = materialDatum.ambient;
				}
				else if (materialDatum.emmisive.grayscale != 0f)
				{
					materialDatum.diffuse = materialDatum.emmisive;
				}
				else
				{
					materialDatum.diffuse = new Color(0.5f, 0.5f, 0.5f, 1f);
				}
			}
			if (materialDatum.alpha != 1f || materialDatum.alphacolor.a != 1f)
			{
				if (materialDatum.alphacolor.a != 1f)
				{
					materialDatum.alpha = materialDatum.alphacolor.a;
				}
				else
				{
					materialDatum.alpha = 1f - materialDatum.alpha;
				}
			}
			else if (materialDatum.alphacolor.grayscale < 1f)
			{
				materialDatum.alpha = 1f - materialDatum.alphacolor.grayscale;
			}
			if (materialDatum.specular.grayscale > 0f)
			{
				materialDatum.specular = new Color(1f - materialDatum.specular.r, 1f - materialDatum.specular.g, 1f - materialDatum.specular.b, 1f);
				if (materialDatum.shininess == 2f)
				{
					materialDatum.shininess = 0.5f;
				}
				else if (materialDatum.shininess >= 2f)
				{
					materialDatum.specular = new Color(1f - materialDatum.specular.r, 1f - materialDatum.specular.g, 1f - materialDatum.specular.b, 1f);
					materialDatum.shininess = (Mathf.Clamp(materialDatum.shininess, 2f, 10f) - 2f) / 7f;
				}
				else if (materialDatum.shininess >= 1f)
				{
					materialDatum.shininess = Mathf.Clamp(materialDatum.shininess, 0f, 1f);
				}
			}
			else if (materialDatum.shininess >= 2f)
			{
				materialDatum.shininess = (Mathf.Clamp(materialDatum.shininess, 2f, 10f) - 2f) / 7f;
				materialDatum.specular = new Color(materialDatum.shininess, materialDatum.shininess, materialDatum.shininess, 1f);
			}
			else if (materialDatum.shininess >= 1f)
			{
				materialDatum.shininess = Mathf.Clamp(materialDatum.shininess, 0f, 1f);
			}
			bool flag = false;
			bool flag2 = false;
			materialDatum.ShaderName = "";
			if (materialDatum.alpha == 1f && materialDatum.alphaTexPath == null)
			{
				if (materialDatum.emmisiveTexPath != null)
				{
					materialDatum.ShaderName += "Self-Illumin/";
					if (TextureList.ContainsKey(materialDatum.emmisiveTexPath))
					{
						texture2D = TMPTextures[TextureList[materialDatum.emmisiveTexPath]];
						flag2 = true;
						flag = true;
					}
				}
				else if (materialDatum.emmisive.grayscale > 0f)
				{
					materialDatum.ShaderName += "Self-Illumin/";
				}
			}
			else if (materialDatum.alpha == 1f)
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
					materialDatum.DiffTexture.name = materialDatum.ID + "_MainTex";
					if (SaveToProject)
					{
						FileIO.SaveTextureToProject(materialDatum.DiffTexture, replace: false);
					}
					materialDatum.DiffTexture.Compress(highQuality: true);
					continue;
				}
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
				materialDatum.EmmisiveTexture.name = materialDatum.ID + "_Illum";
				if (SaveToProject)
				{
					FileIO.SaveTextureToProject(materialDatum.EmmisiveTexture, replace: false);
				}
				materialDatum.EmmisiveTexture.Compress(highQuality: true);
			}
			else if (materialDatum.DiffTexture != null)
			{
				Texture2D texture2D2 = new Texture2D(materialDatum.DiffTexture.width, materialDatum.DiffTexture.height, materialDatum.DiffTexture.format, mipChain: false);
				Color[] pixels3 = materialDatum.DiffTexture.GetPixels(0);
				texture2D2.SetPixels(pixels3, 0);
				materialDatum.DiffTexture = texture2D2;
				materialDatum.DiffTexture.SetPixels(materialDatum.DiffTexture.GetPixels());
				materialDatum.DiffTexture.Apply(updateMipmaps: true);
				materialDatum.DiffTexture.name = materialDatum.ID + "_MainTex";
				if (SaveToProject)
				{
					FileIO.SaveTextureToProject(materialDatum.DiffTexture, replace: false);
				}
				materialDatum.DiffTexture.Compress(highQuality: true);
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
			if (md.emmisive.grayscale > 0f)
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
			material.SetFloat("_Shininess", 0.01f + (md.shininess - 0.01f * md.shininess));
		}
		if (md.DiffTexture != null)
		{
			material.SetTexture("_MainTex", md.DiffTexture);
		}
		return material;
	}

	private void Build()
	{
		Dictionary<string, Material> dictionary = new Dictionary<string, Material>();
		if (hasmaterials)
		{
			foreach (MaterialData materialDatum in materialData)
			{
				if (!dictionary.ContainsKey(materialDatum.ID))
				{
					dictionary.Add(materialDatum.ID, GetMaterial(materialDatum));
				}
			}
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
		GameObject[] array2;
		if (!treatasoneobject)
		{
			array2 = new GameObject[sceneTransforms.Count];
			int num = 0;
			GameObject[] array3 = array;
			foreach (GameObject Geo in array3)
			{
				foreach (SceneGeo item in sceneGeo.FindAll((SceneGeo item) => item.GID == Geo.name))
				{
					if (item.isInstance)
					{
						GameObject gameObject3 = UnityEngine.Object.Instantiate(Geo);
						gameObject3.transform.parent = base.gameObject.transform;
						gameObject3.name = item.ParentNode;
						Material[] array4 = new Material[item.GMaterial.Count];
						for (int k = 0; k < item.GMaterial.Count; k++)
						{
							array4[k] = dictionary[item.GMaterial[k].id];
							array4[k].name = item.GMaterial[k].id;
						}
						gameObject3.GetComponent<Renderer>().materials = array4;
						array2[num] = gameObject3;
						num++;
					}
					else
					{
						Geo.name = item.ParentNode;
						array2[num] = Geo;
						num++;
					}
				}
			}
			foreach (SceneTransform sceneTransform2 in sceneTransforms)
			{
				if (!sceneTransform2.hasgeometry)
				{
					GameObject gameObject4 = new GameObject();
					gameObject4.name = sceneTransform2.NodeName;
					gameObject4.transform.parent = base.gameObject.transform;
					array2[num] = gameObject4;
					num++;
				}
			}
			array3 = array2;
			foreach (GameObject go in array3)
			{
				bool flag = false;
				bool flag2 = false;
				SceneTransform sceneTransform = sceneTransforms.Find((SceneTransform item) => item.NodeName == go.name);
				if (sceneTransform.ParentNode != null)
				{
					GameObject[] array5 = array2;
					foreach (GameObject gameObject5 in array5)
					{
						if (sceneTransform.ParentNode == gameObject5.name)
						{
							go.transform.parent = gameObject5.transform;
							flag = true;
						}
					}
					if (!flag)
					{
						go.transform.parent = base.gameObject.transform;
					}
				}
				else
				{
					flag2 = true;
				}
				go.transform.localRotation = sceneTransform.rot;
				if (flag2)
				{
					go.transform.localPosition = sceneTransform.pos * MasterScale;
				}
				else
				{
					go.transform.localPosition = sceneTransform.pos * MasterScale;
				}
				go.transform.localScale = sceneTransform.scl;
			}
		}
		else
		{
			array[0].transform.localPosition -= MainOffset * MasterScale;
			array2 = new GameObject[1] { array[0] };
		}
		FileIO.LastLoaded = array2;
	}

	private float cf(string v)
	{
		return Convert.ToSingle(v.Trim(), new CultureInfo("en-US"));
	}

	private int ci(string v)
	{
		return Convert.ToInt32(v.Trim(), new CultureInfo("en-US"));
	}

	private static string RemoveWS(string p)
	{
		StringBuilder stringBuilder = new StringBuilder(p);
		stringBuilder.Replace("\r", " ");
		stringBuilder.Replace("\n", " ");
		return Regex.Replace(stringBuilder.ToString(), "\\s+", " ").Trim();
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
