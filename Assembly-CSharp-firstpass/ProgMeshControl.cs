using UnityEngine;

public class ProgMeshControl : MonoBehaviour
{
	private ProgMesh PM;

	private Mesh mesh;

	private float ratio = 100f;

	private float oldratio = 100f;

	private MeshFilter msf;

	private Mesh originalMesh;

	public GUISkin FileIO_Skin;

	private int DialogueWidth = 350;

	private int DialogueHeight = 150;

	private int Dialoguex = 100;

	private int Dialoguey = 100;

	private Rect DialogueBox;

	private bool alteredMesh;

	private void Init()
	{
		oldratio = ratio;
		PM = new ProgMesh();
		mesh = GetComponent<MeshFilter>().mesh;
		PM.CalculatePM(mesh);
	}

	public void UpdateMesh()
	{
		PM.UpdateMesh(ratio);
		mesh.Clear();
		mesh.vertices = PM.finalvertices;
		mesh.normals = PM.finalnormals;
		mesh.triangles = PM.finaltriangles;
		GetComponent<MeshFilter>().mesh = mesh;
	}

	private void Start()
	{
		DialogueBox = new Rect(Dialoguex, Dialoguey, DialogueWidth, DialogueHeight);
		Init();
	}

	private void Update()
	{
		if (PM.Calculated && !PM.Calculating && alteredMesh)
		{
			UpdateMesh();
			alteredMesh = false;
		}
	}

	private void OnGUI()
	{
		DialogueBox = GUI.Window(0, DialogueBox, Display, "");
	}

	private void Display(int windowID)
	{
		GUILayout.BeginArea(new Rect(10f, 10f, DialogueWidth - 20, DialogueHeight - 20));
		GUI.Label(new Rect(0f, 0f, 200f, 25f), PM.render_num.ToString());
		if (!PM.Calculating)
		{
			ratio = GUI.HorizontalSlider(new Rect(50f, 15f, 300f, 45f), ratio, 0f, 100f);
			if (oldratio != ratio)
			{
				oldratio = ratio;
				alteredMesh = true;
			}
		}
		if (GUI.Button(new Rect(0f, 60f, 100f, 40f), "Add Collider"))
		{
			base.transform.gameObject.AddComponent<MeshCollider>();
			GetComponent<MeshCollider>().sharedMesh = GetComponent<MeshFilter>().mesh;
		}
		GUILayout.EndArea();
		GUI.DragWindow();
	}
}
