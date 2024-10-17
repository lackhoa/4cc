struct CSG_Primitive : public OpenCSG::Primitive
{
 /// An object of this class contains the OpenGL id of a display
 /// list that is compiled by the application. render() just invokes
 /// this display list. 
 /// Operation and convexity are just forwarded to the base Primitive class.
 CSG_Primitive(OpenCSG::Operation, u32 convexity);
 
 /// Calls the display list.
 virtual void render();
};

CSG_Primitive::CSG_Primitive(OpenCSG::Operation op, u32 convexity)
: OpenCSG::Primitive(op, convexity)
{
}

struct Test_Painter
{
 Render_Vertex *base;
 i1 count;
};

function void
test_draw_quad(Test_Painter *p, v3 Av, v3 Bv, v3 Cv, v3 Dv)
{// NOTE: vertices are given counter-clock-wise
 argb color = 0xff666666;
 Render_Vertex A = {.pos=V4(Av,1.f), .color=color};
 Render_Vertex B = {.pos=V4(Bv,1.f), .color=color};
 Render_Vertex C = {.pos=V4(Cv,1.f), .color=color};
 Render_Vertex D = {.pos=V4(Dv,1.f), .color=color};
 Render_Vertex vertices[6] = { A,B,C, A,C,D, };
 i1 count = alen(vertices);
 glBufferData(GL_ARRAY_BUFFER, count*sizeof(Render_Vertex), vertices, GL_STREAM_DRAW);
 glDrawArrays(GL_TRIANGLES, 0, count);
}

function void
test_draw_opposite_faces(Test_Painter *p, v3 o, v3 x, v3 y, v3 z)
{
 v3 O = o+z;
 test_draw_quad(p, O-x-y, O+x-y, O+x+y, O-x+y);
 O = o-z;
 test_draw_quad(p, O-x+y, O+x+y, O+x-y, O-x-y);
}

void CSG_Primitive::render()
{
 v3 o = V3();
 v3 x = V3x(1.f);
 v3 y = V3y(1.f);
 v3 z = V3z(1.f);
 
 Test_Painter painter; Test_Painter *p = &painter;
 test_draw_opposite_faces(p, o,x,y,z);
 test_draw_opposite_faces(p, o,z,y,-x);
 test_draw_opposite_faces(p, o,x,z,-y);
}

{//-NOTE: ;opencsg_test
 std::vector<OpenCSG::Primitive*> primitives;
 CSG_Primitive *box = new CSG_Primitive(OpenCSG::Intersection, 1);
 primitives.push_back(box);
 
#define JUST_DRAW_PRIMITIVES 0
#if !JUST_DRAW_PRIMITIVES
 OpenCSG::setOption(OpenCSG::AlgorithmSetting, OpenCSG::SCS);
 OpenCSG::setOption(OpenCSG::DepthComplexitySetting, OpenCSG::NoDepthComplexitySampling);
 // NOTE(kv): Depth pre-pass
 OpenCSG::render(primitives);
 
 {
  // NOTE(kv): Render the primitives
  glDepthFunc(GL_EQUAL);
  for (std::vector<OpenCSG::Primitive*>::const_iterator it = primitives.begin();
       it != primitives.end();
       ++it)
  {
   (*it)->render();
  }
  glDepthFunc(GL_LEQUAL);
 }
#else
 for (std::vector<OpenCSG::Primitive*>::const_iterator it = primitives.begin();
      it != primitives.end();
      ++it)
 {
  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);
  
  (*it)->render();  // opencsg_test.cpp
  
  glDisable(GL_CULL_FACE);
 }
#endif
 
 delete box;
 primitives.clear();
}        
