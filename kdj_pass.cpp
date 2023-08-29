#include "llvm/Pass.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/DataLayout.h"

#include <unordered_set>
#include <iostream>
#include <vector>
#include <sstream>
#include <regex>


using namespace llvm;
using namespace std;

struct Node{
    vector<string> Trace_buf;
    vector<string> Trace_count;
    size_t buf_len;
    size_t count_len;
};

namespace{

    struct KDJPass:public ModulePass{

        static char ID;
        KDJPass():ModulePass(ID){}

        string v2String(Value *value){
            string str;
            llvm::raw_string_ostream rso(str);
            value->print(rso);

            return str;
        }

        string i2String(Instruction *inst){
            string str;
            llvm::raw_string_ostream rso(str);
            inst->print(rso);

            return str;
        }
        
        vector<string> split(string str){
            vector <string> v;
            stringstream ss(str);
            string tmp;
            while (ss>>tmp) v.push_back(tmp);
            return v;
        }

        bool runOnModule(Module &M) override{

            vector<Node> master;
            vector<Node>::iterator it;
            vector<string>::iterator it2;
            unordered_set<string> blacklisted_functions={"gets","__isoc99_scanf","strcpy","sprintf"};
            
            for (Function &F : M) {
                for(BasicBlock &BB : F){
                    for(Instruction &I : reverse(BB)){
                        if(isa<GetElementPtrInst>(I)){ //create node
                            // errs()<<I<<"\n";
                            string str=i2String(&I);
                            str=regex_replace(str,regex("[\\[\\],]"),"");
                            vector <string> v= split(str);
                            string addr = v[0];
                            string ssize=v[4];
                            size_t buf_len=stoi(ssize);
                            // cout<<addr<<"\n";

                            for(it=master.begin();it!=master.end();it++){
                                auto check=find(it->Trace_buf.begin(),it->Trace_buf.end(),addr);
                                if (check!=it->Trace_buf.end()){
                                    it->buf_len=buf_len;
                                }
                            }
                            // auto it = find(Trace_buf.begin(),Trace_buf.end(),addr);
                            // if (it==Trace_buf.end()){

                            // }else{ //check overflow
                            //     cout<<"Buf Size: "<<buf_len<<"\n";
                            // }
                        }
                        else if (isa<LoadInst>(I)){ //connect nodd
                            string str=i2String(&I);
                            // errs()<<I<<"\n";

                            str=regex_replace(str,regex("[,]"),"");
                            vector <string> v=split(str);

                            string dest=v[0];
                            string src=v[5];

                            for(it=master.begin();it!=master.end();it++){
                                auto check=find(it->Trace_buf.begin(),it->Trace_buf.end(),dest);
                                if (check!=it->Trace_buf.end()){
                                    it->Trace_buf.push_back(src);
                                }
                            }

                            // auto it = find(Trace_buf.begin(),Trace_buf.end(),dest);
                            // if (it==Trace_buf.end()){

                            // }
                            // else{
                            //     Trace_buf.push_back(src);
                            // }

                            for(it=master.begin();it!=master.end();it++){
                                auto check=find(it->Trace_count.begin(),it->Trace_count.end(),dest);
                                if (check!=it->Trace_count.end()){
                                    it->Trace_count.push_back(src);
                                }
                            }
                            // auto it2 = find(Trace_data.begin(),Trace_data.end(),dest);
                            // if(it2==Trace_data.end()){

                            // }else{
                            //     Trace_data.push_back(src);
                            // }
                        }
                        else if (isa<StoreInst>(I)){ //connect node
                            Value *val=I.getOperand(0);
                            string str=i2String(&I);

                            str=regex_replace(str,regex("[,]"),"");
                            vector<string> v=split(str);
                            
                            string dest=v[4];
                            string src=v[2];

                            for(it=master.begin();it!=master.end();it++){
                                auto check=find(it->Trace_buf.begin(),it->Trace_buf.end(),dest);
                                if (check!=it->Trace_buf.end()){
                                    it->Trace_buf.push_back(src);
                                }
                            }
                            // auto it = find(Trace_buf.begin(),Trace_buf.end(),dest);
                            // if (it==Trace_buf.end()){

                            // }
                            // else{
                            //     Trace_buf.push_back(src);
                            // }

                            for(it=master.begin();it!=master.end();it++){
                                auto check=find(it->Trace_count.begin(),it->Trace_count.end(),dest);
                                if (check!=it->Trace_count.end()){
                                    if (ConstantInt *CInt=dyn_cast<ConstantInt>(val)){
                                        it->count_len=CInt->getSExtValue();
                                    }else{
                                        it->Trace_count.push_back(src);
                                    }
                                }
                            }
                            // auto it2= find(Trace_data.begin(),Trace_data.end(),dest);
                            // if (it2==Trace_data.end()){
                            // }else{
                            //     if (ConstantInt *CInt=dyn_cast<ConstantInt>(val)){
                            //         size=CInt->getSExtValue();
                            //         errs()<<"Read Size: "<<size<<"\n";
                            //     }else{
                            //         Trace_data.push_back(src);
                            //     }
                            // }
                        }
                        else if (isa<CallInst>(I)){
                            // errs()<<I<<"\n";
                            if (CallInst *CI = dyn_cast<CallInst>(&I)){

                                Function *calledFunction=CI->getCalledFunction();
                                if (calledFunction->getName().str()=="read"){
                                    // call devel's function
                                    // IRBuilder<> IRB;
                                    // IRB.createCall();
                                    

                                    Value *buf = CI->getArgOperand(1);
                                    // errs()<<*buf<<"\n";
                                    string str=v2String(buf);
                                    vector<string> v=split(str);
                                    string buf_name=v[0];
                                    // errs()<<"buf_name: "<<buf_name<<"\n";

                                    Node n;
                                    n.Trace_buf.push_back(buf_name);
                                    
                                    Value *sizeArg=CI->getArgOperand(2);
                                    if (ConstantInt *CInt=dyn_cast<ConstantInt>(sizeArg)){
                                        size_t size=CInt->getSExtValue();
                                        // errs()<<"Read Size: "<<size<<"\n";
                                        n.count_len=size;
                                    }else{
                                        Value *op = CI->getOperand(2);
                                        string str = v2String(op);
                                        vector<string> v=split(str);
                                        string count_name=v[4];
                                        n.Trace_count.push_back(count_name);
                                    }
                                    master.push_back(n);
                                }
                            }
                            
                        }
                    }
                }
            }


            //dump master
            
            for(it=master.begin();it!=master.end();it++){
                // cout<<"Trace Buf"<<"\n";
                // for(it2=it->Trace_buf.begin();it2!=it->Trace_buf.end();it2++){
                //     cout<<*it2<<" ";
                // }
                // cout<<endl;
                // cout<<"Trace Count"<<"\n";
                // for(it2=it->Trace_count.begin();it2!=it->Trace_count.end();it2++){
                //     cout<<*it2<<" ";
                // }
                // cout<<endl;
                if (it->buf_len<it->count_len){
                    cout<<"[*] Stack Buffer Overflow Detect!!!\n";
                    cout<<"Read "<<it->count_len<<" bytes in "<<it->buf_len<<"-byte buffer"<<"\n";
                }
                // cout<<"Buf Length: "<<it->buf_len<<"\n";
                // cout<<"Count Length: "<<it->count_len<<"\n";
            }
            
            return false;
        }
    };
}

char KDJPass::ID=0;
static RegisterPass<KDJPass> X("KDJPass","I am KDJ Hello!",false,false);
