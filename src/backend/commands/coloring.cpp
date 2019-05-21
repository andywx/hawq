#include <stdio.h>
#include <stdlib.h>
#include "string.h"
#include<string>
#include <map>
#include <set>
#include <vector>
#include<assert.h>
#include<algorithm>
#include<time.h>
#include"commands/coloring.h"
#include "nodes/nodes.h"
using namespace std;

#define LINE 1024
typedef unsigned long u32;

//ORIGINAL GRAPH MAP
map<string,set<string> > sp;
map<string,set<string> > op;
map<string,map<string,string> > spo;
map<string,map<string,string> > ops;
//PP GRAPH MAP
map<string,set<string> > spp;
map<string,set<string> > opp;
//VERTEX RANK
map<string,int> s_p_rank;
map<string,int> o_p_rank;
//MARK COLORED VERTEX
map<string,bool> s_p_colored;
map<string,bool> o_p_colored;
//S,P,O
set<string> pro;
set<string> sub;
set<string> obj;
//COUNT COLORS
int color_s=0;
int color_o=0;

//RANKED GRAPH VERTEX
typedef pair<string, int> PAIR;
vector<PAIR> s_p_rank_vec;
vector<PAIR> o_p_rank_vec;

//COMPARE FUNCTION FOR VECTEX RANK
bool cmp_by_value(const PAIR& lhs, const PAIR& rhs) {
  return lhs.second > rhs.second;
}

struct CmpByValue {
  bool operator()(const PAIR& lhs, const PAIR& rhs) {
    return lhs.second > rhs.second;
  }
};

void SP_OP_MAP(char *fileBuffer,DS_RS_LIST** ds,DS_RS_LIST** rs) {
	//A line is pBegin to pEnd
	char *pBegin = fileBuffer;
	char *pEnd = strchr(pBegin, '\n');
	//FLAGS FOR S,P,O
	char *se,*ps,*pe,*os,*oe,*o_flag,*o_flag2;

	*ds=(DS_RS_LIST*)malloc(sizeof(DS_RS_LIST));
	(*ds)->elm=NULL;
	(*ds)->l_id=NULL;
	(*ds)->next=NULL;
	DS_RS_LIST* ds_tmp=(*ds);
	*rs=(DS_RS_LIST*)malloc(sizeof(DS_RS_LIST));
	(*rs)->elm=NULL;
	(*rs)->l_id=NULL;
	(*rs)->next=NULL;
	DS_RS_LIST* rs_tmp=(*rs);

	int count_ds=0;
	int count_rs=0;

    while (pEnd)
    	{


    		se=strchr(pBegin,'>'),
    		ps=strchr(se,'<'),
    		pe=strchr(ps,'>');


    		string s;
    		s.insert(0, pBegin+1, se-pBegin-1);
    		assert(!s.empty());

    		string p;
    		p.insert(0, ps+1, pe-ps-1);
    		assert(!p.empty());

    		string o;
    		o_flag=strchr(pe,'\"');
    		if(o_flag)
    		{
    			o_flag2=strchr(pe,'<');
    			if(o_flag2)
    			{
    				bool k=strlen(o_flag)<strlen(o_flag2);

					if(k)
					{
						os=strchr(pe,'<');
						oe=strchr(os,'>');
						o.insert(0, os+1, oe-os-1);
						assert(!o.empty());

					}else{
						os=strchr(pe,'\"');
						oe=strchr(os+1,'\"');
						o.insert(0, os+1, oe-os-1);
						assert(!o.empty());
					}
    			}else{
					os=strchr(pe,'\"');
					oe=strchr(os+1,'\"');
					o.insert(0, os+1, oe-os-1);
					assert(!o.empty());
				}

    		}else
    		{
    			os=strchr(pe,'<');
    			oe=strchr(os,'>');
    			o.insert(0, os+1, oe-os-1);
    			assert(!o.empty());
    		}


    		sub.insert(s);
    		pro.insert(p);
    		obj.insert(o);

    		set<string> p_flag;
    		map<string,string> p_flag_spo;
    		map<string,string> p_flag_ops;
    		map<string,string>::iterator po;
    		map<string,string>::iterator ps;

    		map<string,set<string> >::iterator iter_m;
    		map<string,map<string,string> >::iterator iter_spo;
    		map<string,map<string,string> >::iterator iter_ops;
    		/*SP INPUT*/
    		iter_m=sp.find(s);
    		iter_spo=spo.find(s);
    		if(iter_m!=sp.end()&&iter_spo!=spo.end())
    		{
    				iter_m->second.insert(p);
    				po=iter_spo->second.find(p);
    				if(po!=iter_spo->second.end())
    				{
    					char* po_flag=(char*)malloc(sizeof(char)*(po->second.length()+1));
    					memset(po_flag,0,sizeof(char)*(po->second.length()+1));
    					po->second.copy(po_flag,po->second.length(),0);
    					if(strstr(po_flag,"l_id:"))
    					{
    						char* elm_flag=(char*)malloc(sizeof(char)*(o.length()+1));
    						memset(elm_flag,0,sizeof(char)*(o.length()+1));
    						o.copy(elm_flag,o.length(),0);
    						ds_tmp->elm=(char*)malloc(sizeof(char)*(o.length()+1));
    						memset(ds_tmp->elm,0,sizeof(char)*(o.length()+1));
    						ds_tmp->elm=elm_flag;
    						ds_tmp->l_id=(char*)malloc(sizeof(char)*(po->second.length()+1));
    						memset(ds_tmp->l_id,0,sizeof(char)*(po->second.length()+1));
    						ds_tmp->l_id=po_flag;
    						ds_tmp->next=(DS_RS_LIST*)malloc(sizeof(DS_RS_LIST));
    						ds_tmp=ds_tmp->next;
    						ds_tmp->l_id=NULL;
    						ds_tmp->elm=NULL;
    						ds_tmp->next=NULL;

    					}else{
    						char l_id_ds[16]="l_id:";
							count_ds++;
    						char count[10]={0};
    						sprintf(count, "%d", count_ds);
    						strcat(l_id_ds,count);
    						char* l_id=(char*)malloc(30*sizeof(char));
    						memset(l_id,0,30*sizeof(char));
    						strcpy(l_id,l_id_ds);
    						ds_tmp->l_id=(char*)malloc(sizeof(char)*30);
    						memset(ds_tmp->l_id,0,30*sizeof(char));
    						ds_tmp->l_id=l_id;
    						char* elm_flag=(char*)malloc(sizeof(char)*(o.length()+1));
    						memset(elm_flag,0,sizeof(char)*(o.length()+1));
    						o.copy(elm_flag,o.length(),0);
    						ds_tmp->elm=(char*)malloc(sizeof(char)*(o.length()+1));
    						memset(ds_tmp->elm,0,sizeof(char)*(o.length()+1));
    						ds_tmp->elm=elm_flag;

    						char*o2=(char*)malloc(1000*sizeof(char));
    						memset(o2,0,1000*sizeof(char));
    						strcpy(o2,po_flag);
    						ds_tmp->next=(DS_RS_LIST*)malloc(sizeof(DS_RS_LIST));
    						ds_tmp=ds_tmp->next;
    						ds_tmp->elm=(char*)malloc(sizeof(char)*1000);
    						memset(ds_tmp->elm,0,1000*sizeof(char));
    						ds_tmp->elm=o2;
    						ds_tmp->l_id=(char*)malloc(sizeof(char)*30);
    						memset(ds_tmp->l_id,0,30*sizeof(char));
    						ds_tmp->l_id=l_id;
    						ds_tmp->next=(DS_RS_LIST*)malloc(sizeof(DS_RS_LIST));
    						ds_tmp=ds_tmp->next;
    						ds_tmp->l_id=NULL;
    						ds_tmp->elm=NULL;
    						ds_tmp->next=NULL;

    						iter_spo->second.erase(po);
    						iter_spo->second.insert(make_pair(p,l_id));
    					}
    				}else{
    					iter_spo->second.insert(make_pair(p,o));
    				}
    		}else{
    				p_flag.insert(p);
    				p_flag_spo.insert(make_pair(p,o));
    				sp.insert(make_pair(s,p_flag));
    				spo.insert(make_pair(s,p_flag_spo));
    		}
    		/*OP INPUT*/
    		p_flag.clear();
    		iter_m=op.find(o);
    		iter_ops=ops.find(o);
    		if(iter_m!=op.end())
    		{
    				iter_m->second.insert(p);
    				ps=iter_ops->second.find(p);
    				if(ps!=iter_ops->second.end())
    				{
    					char* ps_flag=(char*)malloc(sizeof(char)*(ps->second.length()+1));
    					memset(ps_flag,0,sizeof(char)*(ps->second.length()+1));
    					ps->second.copy(ps_flag,ps->second.length(),0);
    					if(strstr(ps_flag,"l_id:"))
    					{
    						rs_tmp->l_id=(char*)malloc(sizeof(char)*(ps->second.length()+1));
    						memset(rs_tmp->l_id,0,sizeof(char)*(ps->second.length()+1));
    						rs_tmp->l_id=ps_flag;
    						char* elm_flag=(char*)malloc(sizeof(char)*(s.length()+1));
    						memset(elm_flag,0,sizeof(char)*(s.length()+1));
    						s.copy(elm_flag,s.length(),0);
    						rs_tmp->elm=(char*)malloc(sizeof(char)*(s.length()+1));
    						memset(rs_tmp->elm,0,sizeof(char)*(s.length()+1));
    						rs_tmp->elm=elm_flag;

    						rs_tmp->next=(DS_RS_LIST*)malloc(sizeof(DS_RS_LIST));
    						rs_tmp=rs_tmp->next;
    						rs_tmp->l_id=NULL;
    						rs_tmp->elm=NULL;
    						rs_tmp->next=NULL;

    					}else{
    						char l_id_rs[16]="l_id:";
							count_rs++;
							char count[10]={0};
							sprintf(count, "%d", count_rs);
							strcat(l_id_rs,count);
							char* l_id=(char*)malloc(sizeof(char)*30);
							memset(l_id,0,30*sizeof(char));
							strcpy(l_id,l_id_rs);
							rs_tmp->l_id=(char*)malloc(sizeof(char)*30);
    						memset(rs_tmp->l_id,0,30*sizeof(char));
    						rs_tmp->l_id=l_id;

    						rs_tmp->l_id[strlen(l_id)]='\0';

    						char* elm_flag=(char*)malloc(sizeof(char)*(s.length()+1));
    						memset(elm_flag,0,sizeof(char)*(s.length()+1));
    						s.copy(elm_flag,s.length(),0);
    						rs_tmp->elm=(char*)malloc(sizeof(char)*(s.length()+1));
    						memset(rs_tmp->elm,0,sizeof(char)*(s.length()+1));
    						rs_tmp->elm=elm_flag;
    						char* s2=(char*)malloc(100*sizeof(char));
    						memset(s2,0,100*sizeof(char));
    						strcpy(s2,ps_flag);

    						rs_tmp->next=(DS_RS_LIST*)malloc(sizeof(DS_RS_LIST));
    						rs_tmp=rs_tmp->next;
    						rs_tmp->elm=(char*)malloc(sizeof(char)*100);
    						memset(rs_tmp->elm,0,100*sizeof(char));
    						rs_tmp->elm=s2;
    						rs_tmp->l_id=(char*)malloc(sizeof(char)*30);
    						memset(rs_tmp->l_id,0,30*sizeof(char));
    						rs_tmp->l_id=l_id;

    						rs_tmp->next=(DS_RS_LIST*)malloc(sizeof(DS_RS_LIST));
    						rs_tmp=rs_tmp->next;
    						rs_tmp->l_id=NULL;
    						rs_tmp->elm=NULL;
    						rs_tmp->next=NULL;

    						iter_ops->second.erase(ps);
    						iter_ops->second.insert(make_pair(p,l_id));
    					}
    				}else{
    					iter_ops->second.insert(make_pair(p,s));
    				}
    		}else{
    			p_flag.insert(p);
    			p_flag_ops.insert(make_pair(p,s));
    			op.insert(make_pair(o,p_flag));
    			ops.insert(make_pair(o,p_flag_ops));
    		}

    		pBegin = pEnd + 1;
    		pEnd = strchr(pBegin, '\n');//next line
    	}

}

void OPP_MAP()
{
	map<string,set<string> >::iterator iter_m_op;
	map<string,set<string> >::iterator iter_m_pp;
	set<string>::iterator iter_s_obj;
	set<string>::iterator iter_s_p;
	set<string>::iterator iter_s_p_flag;
	set<string> p_flag;
	iter_s_obj=obj.begin();
	while(iter_s_obj!=obj.end())
	{
		iter_m_op=op.find(*iter_s_obj);
		iter_s_p_flag=
				iter_s_p=iter_m_op->second.begin();

		while(iter_s_p!=iter_m_op->second.end())
		{
			while(iter_s_p_flag!=iter_m_op->second.end())
			{
				iter_m_pp=opp.find(*iter_s_p);
				if(iter_m_pp!=opp.end())
				{
					if(*iter_s_p_flag!=*iter_s_p)
						iter_m_pp->second.insert(*iter_s_p_flag);
				}else
				{
					p_flag=iter_m_op->second;
					p_flag.erase(*iter_s_p);
					opp.insert(make_pair(*iter_s_p,p_flag));
				}
				iter_s_p_flag++;
			}
			iter_s_p++;
			iter_s_p_flag=iter_m_op->second.begin();
		}
		iter_s_obj++;
	}
}
void SPP_MAP()
{
	map<string,set<string> >::iterator iter_m_sp;
	map<string,set<string> >::iterator iter_m_pp;
	set<string>::iterator iter_s_sub;
	set<string>::iterator iter_s_p;
	set<string>::iterator iter_s_p_flag;
	set<string> p_flag;
	iter_s_sub=sub.begin();
	while(iter_s_sub!=sub.end())
	{
		iter_m_sp=sp.find(*iter_s_sub);
		iter_s_p_flag=
				iter_s_p=iter_m_sp->second.begin();

		while(iter_s_p!=iter_m_sp->second.end())
		{
			while(iter_s_p_flag!=iter_m_sp->second.end())
			{
				iter_m_pp=spp.find(*iter_s_p);
				if(iter_m_pp!=spp.end())
				{
					if(*iter_s_p_flag!=*iter_s_p)
						iter_m_pp->second.insert(*iter_s_p_flag);
				}else
				{
					p_flag=iter_m_sp->second;
					p_flag.erase(*iter_s_p);
					spp.insert(make_pair(*iter_s_p,p_flag));
				}
				iter_s_p_flag++;
			}
			iter_s_p++;
			iter_s_p_flag=iter_m_sp->second.begin();
		}
		iter_s_sub++;
	}

}

void V_RANK()
{
	/*SP RANK*/
	set<string>::iterator iter_s_p;
	map<string,set<string> >::iterator iter_s_p_size;
	iter_s_p=pro.begin();
	while(iter_s_p!=pro.end())
	{
		iter_s_p_size=spp.find(*iter_s_p);
		s_p_rank.insert(make_pair(*iter_s_p,(int)iter_s_p_size->second.size()));
		s_p_colored.insert(make_pair(*iter_s_p,false));
		iter_s_p++;
	}
	for (map<string, int>::iterator curr = s_p_rank.begin(); curr != s_p_rank.end(); curr++)
	        s_p_rank_vec.push_back(make_pair(curr->first, curr->second));
	sort(s_p_rank_vec.begin(), s_p_rank_vec.end(), CmpByValue());

	/*OP RANK*/
	iter_s_p=pro.begin();
	while(iter_s_p!=pro.end())
	{
		iter_s_p_size=opp.find(*iter_s_p);
		o_p_rank.insert(make_pair(*iter_s_p,(int)iter_s_p_size->second.size()));
		o_p_colored.insert(make_pair(*iter_s_p,false));
		iter_s_p++;
	}
	for (map<string, int>::iterator curr = o_p_rank.begin(); curr != o_p_rank.end(); curr++)
			o_p_rank_vec.push_back(make_pair(curr->first, curr->second));
	sort(o_p_rank_vec.begin(), o_p_rank_vec.end(), CmpByValue());
}
void COLOR_S(){
	map<string,set<string> >::iterator iter_s_p_near;
	map<string,bool >::iterator iter_s_p_colored;
	set<string> colored_s_temp;
	set<string>::iterator iter_s_pp;

	/*SP COLOR*/
	int curr=0;
	int vecsize=(int)s_p_rank_vec.size();
	while(curr!=vecsize)
	{
		string p_curr=s_p_rank_vec[curr].first;
		iter_s_p_colored=s_p_colored.find(p_curr);
		colored_s_temp.clear();
		while(iter_s_p_colored->second==true)
		{
			curr++;
			if(curr==vecsize-1)
				break;
			p_curr=s_p_rank_vec[curr].first;
			iter_s_p_colored=s_p_colored.find(p_curr);
		}
		if(iter_s_p_colored->second==false)
		{
			color_s++;
			iter_s_p_colored->second=true;
			colored_s_temp.insert(p_curr);
		}

		int flag=curr+1;
		while(flag!=(int)s_p_rank_vec.size())
		{
			string p_flag=s_p_rank_vec[flag].first;

			set<string> near_flag;
			near_flag=spp.find(p_flag)->second;
			set<string>::iterator colored_p=colored_s_temp.begin();
			bool near=false;
			while(colored_p!=colored_s_temp.end())
			{
				if(near_flag.find(*colored_p)!=near_flag.end())
				{
					near=true;
					break;
				}
				colored_p++;
			}
			if(near==false&&(s_p_colored.find(p_flag)->second==false))
			{
				s_p_colored.find(p_flag)->second=true;
				colored_s_temp.insert(p_flag);
			}
			flag++;
		}
		curr++;
	}


}
void COLOR_O(){
	map<string,set<string> >::iterator iter_o_p_near;
	map<string,bool >::iterator iter_o_p_colored;
	set<string> colored_o_temp;
	set<string>::iterator iter_o_pp;

	/*OP COLOR*/
	int curr=0;
	while(curr!=(int)o_p_rank_vec.size())
	{
		string p_curr=o_p_rank_vec[curr].first;
		iter_o_p_colored=o_p_colored.find(p_curr);
		colored_o_temp.clear();
		while(iter_o_p_colored->second==true)
		{
			curr++;
			if(curr==(int)o_p_rank_vec.size()-1)
				break;
			p_curr=o_p_rank_vec[curr].first;
			iter_o_p_colored=o_p_colored.find(p_curr);
		}
		if(iter_o_p_colored->second==false)
		{
			color_o++;
			iter_o_p_colored->second=true;
			colored_o_temp.insert(p_curr);
		}

		int flag=curr+1;
		while(flag!=(int)o_p_rank_vec.size())
		{
			string p_flag=o_p_rank_vec[flag].first;

			set<string> near_flag;
			near_flag=opp.find(p_flag)->second;
			set<string>::iterator colored_p=colored_o_temp.begin();
			bool near=false;
			while(colored_p!=colored_o_temp.end())
			{
				if(near_flag.find(*colored_p)!=near_flag.end())
				{
					near=true;
					break;
				}
				colored_p++;
			}
			if(near==false&&(o_p_colored.find(p_flag)->second==false))
			{
				o_p_colored.find(p_flag)->second=true;
				colored_o_temp.insert(p_flag);
			}
			flag++;
		}
		curr++;
	}
}
int coloring_s(char* file,TRIPLE** triple_s,DS_RS_LIST** ds,DS_RS_LIST** rs) {
	FILE *fp;
	/*INPUT FILE TEST*/
	char* filename = file;

	/*FILE IO*/
	if ((fp = fopen(filename, "r")) == NULL) {
		printf("open file error!!\n");
		return 0;
	}
	fseek(fp, 0, SEEK_END);//FILE BEGIN
	u32 uSize = ftell(fp);//FILE SIZE
	rewind(fp);//POINTER TO FILE BEBIN

	char *fileBuffer = new char[uSize];
	fread(fileBuffer, 1, uSize, fp);//INPUT A CHUNK

	clock_t start,end;
	start=clock();
	SP_OP_MAP(fileBuffer,ds,rs);//ORIGINAL GRAPH
	SPP_MAP();//SP GRAPH
	OPP_MAP();//OP GRAPH
	V_RANK();//VERTEX RANK
	COLOR_S();//MAIN BODY COLOR
	end=clock();

	/*SP_MAP PRINT*/
	map<string,set<string> >::iterator iter_m;
	set<string>::iterator iter_s;

	/*TRIPLE STRUCT CONSTRUCT*/
	map<string,map<string,string> >::iterator iter_write;
	map<string,string>::iterator iter_s_write;
	iter_write=spo.begin();

	*triple_s=(TRIPLE*)malloc(sizeof(TRIPLE));
	(*triple_s)->next=NULL;
	(*triple_s)->sub_pre_obj.s=NULL;
	(*triple_s)->sub_pre_obj.values=(P2O*)malloc(sizeof(P2O));
	(*triple_s)->sub_pre_obj.values->p=NULL;
	(*triple_s)->sub_pre_obj.values->o=NULL;
	(*triple_s)->sub_pre_obj.values->next=NULL;
	TRIPLE* triple_s_tmp=(*triple_s);
	P2O* p2o_tmp=triple_s_tmp->sub_pre_obj.values;
	while(iter_write!=spo.end())
	{
		iter_s_write=iter_write->second.begin();
		triple_s_tmp->sub_pre_obj.s=(char*)iter_write->first.c_str();

		while(iter_s_write!=iter_write->second.end())
		{
			char* pre=(char*)iter_s_write->first.c_str();
			char* obj=(char*)iter_s_write->second.c_str();
			p2o_tmp->p=pre;
			p2o_tmp->o=obj;
			p2o_tmp->next=(P2O*)malloc(sizeof(P2O));
			p2o_tmp=p2o_tmp->next;
			p2o_tmp->p=NULL;
			p2o_tmp->o=NULL;
			p2o_tmp->next=NULL;
			iter_s_write++;
		}
		triple_s_tmp->next=(TRIPLE*)malloc(sizeof(TRIPLE));
		triple_s_tmp=triple_s_tmp->next;
		triple_s_tmp->next=NULL;
		triple_s_tmp->sub_pre_obj.s=NULL;
		triple_s_tmp->sub_pre_obj.values=(P2O*)malloc(sizeof(P2O));
		triple_s_tmp->sub_pre_obj.values->p=NULL;
		triple_s_tmp->sub_pre_obj.values->o=NULL;
		triple_s_tmp->sub_pre_obj.values->next=NULL;
		p2o_tmp=triple_s_tmp->sub_pre_obj.values;
		iter_write++;
	}

	return color_s;
}
int coloring_o(char* file,TRIPLE_DIV** triple_o)
{
	COLOR_O();
	map<string,string>::iterator iter_o_write;
	map<string,map<string,string> >::iterator iter_write;
	iter_write=ops.begin();

	(*triple_o)=(TRIPLE_DIV*)malloc(sizeof(TRIPLE_DIV));
	(*triple_o)->next=NULL;
	(*triple_o)->obj_sub_pre.o=NULL;
	(*triple_o)->obj_sub_pre.values=(P2S*)malloc(sizeof(P2S));
	(*triple_o)->obj_sub_pre.values->p=NULL;
	(*triple_o)->obj_sub_pre.values->s=NULL;
	(*triple_o)->obj_sub_pre.values->next=NULL;
	TRIPLE_DIV* triple_o_tmp=(*triple_o);
	P2S* p2s_tmp=triple_o_tmp->obj_sub_pre.values;
	while(iter_write!=ops.end())
	{
		iter_o_write=iter_write->second.begin();
		triple_o_tmp->obj_sub_pre.o=(char*)iter_write->first.c_str();

		while(iter_o_write!=iter_write->second.end())
		{
			char* pre=(char*)iter_o_write->first.c_str();
			char* sub=(char*)iter_o_write->second.c_str();
			p2s_tmp->p=pre;
			p2s_tmp->s=sub;
			p2s_tmp->next=(P2S*)malloc(sizeof(P2S));
			p2s_tmp=p2s_tmp->next;
			p2s_tmp->p=NULL;
			p2s_tmp->s=NULL;
			p2s_tmp->next=NULL;
			iter_o_write++;
		}
		triple_o_tmp->next=(TRIPLE_DIV*)malloc(sizeof(TRIPLE_DIV));
		triple_o_tmp=triple_o_tmp->next;
		triple_o_tmp->next=NULL;
		triple_o_tmp->obj_sub_pre.o=NULL;
		triple_o_tmp->obj_sub_pre.values=(P2S*)malloc(sizeof(P2S));
		triple_o_tmp->obj_sub_pre.values->p=NULL;
		triple_o_tmp->obj_sub_pre.values->s=NULL;
		triple_o_tmp->obj_sub_pre.values->next=NULL;
		p2s_tmp=triple_o_tmp->obj_sub_pre.values;
		iter_write++;
	}
	return color_o;
}
int DoColoring_s(char* file,TRIPLE** triple_s,DS_RS_LIST** ds,DS_RS_LIST** rs){
	return coloring_s(file,triple_s,ds,rs);
}
int DoColoring_o(char* file,TRIPLE_DIV** triple_o){
	return coloring_o(file,triple_o);
}
