// ConfigNode API behaviour (ConfigNode.cs), beyond the byte-exact golden corpus.
#include <limits>

#include "doctest/doctest.h"
#include "ksp/config/config_node.hpp"

using namespace ksp;

TEST_CASE("Parse: comments, braces on value lines, ids, stray close brace") {
    auto n = ConfigNode::Parse("a = 1 // c\nurl = http://x/y\nPART (abc)\n{\nname = p\nMODULE { x = 2 }\n}\n}\nlost = 1\n");
    REQUIRE(n.get() != nullptr);
    CHECK(n->name == "root");
    CHECK(*n->GetValue("a") == "1");
    CHECK(*n->GetValue("url") == "http:");
    CHECK_FALSE(n->HasValue("lost"));
    ConfigNode* part = n->GetNode("PART");
    REQUIRE(part);
    CHECK(part->id == "abc");
    CHECK(*part->GetNode("MODULE")->GetValue("x") == "2");
    CHECK(n->ToString().find("PART (abc)\n") != std::string::npos);
    CHECK(n->SaveToString().find("(abc)") == std::string::npos);
    CHECK(n->SaveToString() == "a = 1\r\nurl = http:\r\nPART\r\n{\r\n\tname = p\r\n\tMODULE\r\n\t{\r\n\t\tx = 2\r\n\t}\r\n}\r\n");
}

TEST_CASE("Value sanitizing and CleanupInput") {
    ConfigNode n("N");
    n.AddValue("a{b}=c", "v{1}\tx\n");
    CHECK(n.values()[0].name == "a[b]-c");
    CHECK(n.values()[0].value == "v[1] x");
    n.SetValue("a[b]-c", "q}", false);
    CHECK(n.values()[0].value == "q]");
}

TEST_CASE("SetValue quirks: returns false when creating, created values are not sanitized") {
    ConfigNode n("N");
    CHECK_FALSE(n.SetValue("k", "{v}", true));
    CHECK(*n.GetValue("k") == "{v}");
    CHECK(n.SetValue("k", "w"));
    CHECK(*n.GetValue("k") == "w");
    CHECK_FALSE(n.SetValue("missing", "x"));
}

TEST_CASE("RemoveValue removes the last match; RemoveValues(string) removes exact names") {
    ConfigNode n("N");
    n.AddValue("k", "1");
    n.AddValue("k", "2");
    n.AddValue("kk", "3");
    n.RemoveValue("k");
    CHECK(n.GetValues("k") == std::vector<std::string>{"1"});
    n.RemoveValues(std::string_view("k"));
    CHECK(n.GetValues("k").empty());
    CHECK(n.HasValue("kk"));
    n.RemoveValuesStartWith("k");
    CHECK_FALSE(n.HasValue("kk"));
}

TEST_CASE("Node lookup by name, index, id and value") {
    auto n = ConfigNode::Parse("A\n{\nname = x\n}\nA\n{\nname = y\n}\nB (id7)\n{\n}\n");
    CHECK(n->GetNode("A", 1)->GetValue("name") == "y");
    CHECK(n->GetNode("A", "name", std::string("y")) == n->GetNode("A", 1));
    CHECK(n->GetNodeID("id7")->name == "B");
    CHECK(n->GetNodes("A").size() == 2);
    CHECK(n->nodes().CountByName("A") == 2);
    CHECK(n->AddNode("   ") == nullptr);
    ConfigNode* c = n->AddNode("NAME extra");
    CHECK(c->name == "NAME");
    CHECK(c->id == "extra");
}

TEST_CASE("Typed AddValue / TryGetValue") {
    ConfigNode n("N");
    n.AddValue("f", 0.1f);
    n.AddValue("d", 0.1);
    n.AddValue("b", true);
    n.AddValue("i", 42);
    n.AddValue("v", Vector3f(1, 2.5f, -3));
    n.AddValue("vd", Vector3d(1, 0.1, 1e20));
    n.AddValue("q", QuaternionD(0, 0, 0, 1));
    n.AddValue("c32", Color32(1, 2, 3, 4));
    CHECK(*n.GetValue("f") == "0.100000001");
    CHECK(*n.GetValue("d") == "0.10000000000000001");
    CHECK(*n.GetValue("b") == "True");
    CHECK(*n.GetValue("v") == "1,2.5,-3");
    CHECK(*n.GetValue("vd") == "1,0.10000000000000001,1E+20");
    CHECK(*n.GetValue("q") == "0,0,0,1");
    CHECK(*n.GetValue("c32") == "1,2,3,4");

    float f = 0;
    CHECK(n.TryGetValue("f", f));
    CHECK(f == 0.1f);
    double d = 0;
    CHECK(n.TryGetValue("d", d));
    CHECK(d == 0.1);
    bool b = false;
    CHECK(n.TryGetValue("b", b));
    CHECK(b);
    int32_t i = 0;
    CHECK(n.TryGetValue("i", i));
    CHECK(i == 42);
    Vector3f v;
    CHECK(n.TryGetValue("v", v));
    CHECK(v.Equals(Vector3f(1, 2.5f, -3)));
    Vector3d vd;
    CHECK(n.TryGetValue("vd", vd));
    CHECK(vd.z == 1e20);
    int32_t untouched = 9;
    CHECK_FALSE(n.TryGetValue("f", untouched));
    CHECK(untouched == 9);
    CHECK_FALSE(n.TryGetValue("missing", untouched));
    Color32 c;
    CHECK(n.TryGetValue("c32", c));
    CHECK(c.Equals(Color32(1, 2, 3, 4)));

    ConfigNode m("M");
    m.AddValue("col", "2, 0.5, -1");
    m.AddValue("html", "#FF8000");
    m.AddValue("named", "orange");
    Color col;
    CHECK(m.TryGetValue("col", col));
    CHECK(col.Equals(Color(1, 0.5f, 0, 1)));
    CHECK(m.TryGetValue("html", col));
    CHECK(col.r == 1.0f);
    CHECK(m.TryGetValue("named", col));
    CHECK(col.g == 165.0f / 255.0f);
}

TEST_CASE("Guid parsing") {
    ConfigNode n("N");
    n.AddValue("g", "0123456789abcdef0123456789ABCDEF");
    n.AddValue("h", "{01234567-89ab-cdef-0123-456789abcdef}");
    n.AddValue("bad", "xyz");
    Guid g, h, bad;
    CHECK(n.TryGetValue("g", g));
    CHECK(g.ToString('N') == "0123456789abcdef0123456789abcdef");
    // ValueList.Add sanitizes '{' '}' to '[' ']', so a braced GUID cannot be stored through AddValue (KSP behaviour)
    CHECK(*n.GetValue("h") == "[01234567-89ab-cdef-0123-456789abcdef]");
    CHECK_FALSE(n.TryGetValue("h", h));
    CHECK(Guid::TryParse("{01234567-89ab-cdef-0123-456789abcdef}", h));
    CHECK(h.ToString() == "01234567-89ab-cdef-0123-456789abcdef");
    CHECK(Guid::TryParse(" (01234567-89ab-cdef-0123-456789abcdef) ", h));
    CHECK_FALSE(n.TryGetValue("bad", bad));
    CHECK(bad == Guid::Empty());
}

TEST_CASE("TryGetEnum matches member names exactly") {
    enum class E { A, B };
    static constexpr std::pair<std::string_view, E> members[] = {{"A", E::A}, {"B", E::B}};
    ConfigNode n("N");
    n.AddValue("e", "B");
    n.AddValue("f", "b");
    E e = E::A;
    CHECK(n.TryGetEnum<E>("e", e, E::A, members));
    CHECK(e == E::B);
    CHECK_FALSE(n.TryGetEnum<E>("f", e, E::A, members));
    CHECK(e == E::A);
}

TEST_CASE("CopyTo / CreateCopy / Merge") {
    auto n = ConfigNode::Parse("a = 1\nX (i)\n{\nb = 2\n}\n");
    auto copy = n->CreateCopy();
    CHECK(copy->ToString() == n->ToString());
    ConfigNode target("T");
    target.AddValue("a", "old");
    n->CopyTo(target, true);
    CHECK(target.GetValues("a") == std::vector<std::string>{"1"});

    auto anon = ConfigNode::Parse("{\nx = 1\n}\n");
    CHECK_THROWS_AS(anon->CreateCopy(), NullReferenceException);  // KSP: NullReferenceException
}

TEST_CASE("Load returns null for missing or empty files") {
    CHECK(ConfigNode::Load("Z:/definitely/missing.cfg").get() == nullptr);
    CHECK(ConfigNode::LoadFromLines({}).get() == nullptr);
    CHECK(ConfigNode::LoadFromLines({"// only comment"}).get() == nullptr);
    CHECK(ConfigNode::Parse("").get() != nullptr);
}

TEST_CASE("ParseMatrix4x4 keeps KSP's index bug") {
    const Matrix4x4f m = ConfigNode::ParseMatrix4x4("1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16");
    CHECK(m.m11 == 7.0f);
    CHECK(m.m12 == 8.0f);
    CHECK(m.m13 == 0.0f);
    CHECK(m.m33 == 16.0f);
}
