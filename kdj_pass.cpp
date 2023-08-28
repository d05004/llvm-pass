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

            vector<string> Trace;
            unordered_set<string> blacklisted_functions={"gets","__isoc99_scanf","strcpy","sprintf"};
            unsigned int size;
            unsigned int buf_len;
            

            for (Function &F : M) {
                auto *DL=&F.getParent()->getDataLayout();
                for(BasicBlock &BB : F){
                    for(Instruction &I : reverse(BB)){
                        if(isa<GetElementPtrInst>(I)){ //create node
                            // errs()<<I<<"\n";
                            string str=i2String(&I);
                            str=regex_replace(str,regex("[\\[\\],]"),"");
                            vector <string> v= split(str);
                            string addr = v[0];
                            string ssize=v[4];
                            buf_len=stoi(ssize);
                            // cout<<addr<<"\n";
                            auto it = find(Trace.begin(),Trace.end(),addr);
                            if (it==Trace.end()){

                            }else{ //check overflow
                                cout<<"Buf Size: "<<buf_len<<"\n";
                                cout<<"Read Size: "<<size<<"\n";
                                if (size>buf_len){
                                    cout<<"[*]BOF DETECTED!"<<"\n";
                                }
                            }
                        }
                        else if (isa<LoadInst>(I)){ //connect nodd
                            string str=i2String(&I);
                            // errs()<<I<<"\n";

                            str=regex_replace(str,regex("[,]"),"");
                            vector <string> v=split(str);

                            string dest=v[0];
                            string src=v[5];

                            auto it = find(Trace.begin(),Trace.end(),dest);
                            if (it==Trace.end()){

                            }
                            else{
                                Trace.push_back(src);
                            }
                        }
                        else if (isa<StoreInst>(I)){ //connect node
                            // errs()<<I<<"\n";
                            string str=i2String(&I);

                            str=regex_replace(str,regex("[,]"),"");
                            vector<string> v=split(str);
                            
                            string dest=v[4];
                            string src=v[2];

                            auto it = find(Trace.begin(),Trace.end(),dest);
                            if (it==Trace.end()){

                            }
                            else{
                                Trace.push_back(src);
                                // cout<<"store "<<src<<" "<<dest<<"\n";
                            }
                        }
                        else if (isa<CallInst>(I)){
                            // errs()<<I<<"\n";
                            if (CallInst *CI = dyn_cast<CallInst>(&I)){

                                Function *calledFunction=CI->getCalledFunction();
                                if (calledFunction->getName().str()=="read"){

                                    Value *buf = CI->getArgOperand(1);
                                    // errs()<<*buf<<"\n";
                                    string str=v2String(buf);
                                    vector<string> v=split(str);
                                    string buf_name=v[0];
                                    // errs()<<"buf_name: "<<buf_name<<"\n";
                                    Trace.push_back(buf_name);

                                    Value *sizeArg=CI->getArgOperand(2);
                                    ConstantInt *CInt = cast<ConstantInt>(sizeArg);
                                    size=CInt->getSExtValue();
                                    // errs()<<buf_len<<"\n";
                                }
                            }
                            
                        }
                    }
                }
            }

            
            return false;
        }
    };
}

char KDJPass::ID=0;
static RegisterPass<KDJPass> X("KDJPass","I am KDJ Hello!",false,false);
