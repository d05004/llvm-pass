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

        bool runOnModule(Module &M) override{

            unordered_set<string> blacklisted_functions={"gets","__isoc99_scanf","strcpy","sprintf"};
            for (Function &F : M) {
                auto *DL=&F.getParent()->getDataLayout();
                for(BasicBlock &BB : F){
                    for(Instruction &I : BB){
                        if(isa<AllocaInst>(I)){ //create node
                            //errs()<<I<<"\n";
                        }
                        else if (isa<LoadInst>(I)){ //connect nodd
                            string str;
                            
                            llvm::raw_string_ostream rso(str);
                            I.print(rso);

                            str=regex_replace(str,regex("[,]"),"");
                            vector <string> v;
                            stringstream ss(str);
                            string tmp;
                            while (ss>>tmp) v.push_back(tmp);

                            cout<<"load "<<v[0] <<" "<<v[5] <<"\n";

                        }
                        else if (isa<StoreInst>(I)){ //connect node
                            //errs()<<I<<"\n";
                            string str;
                            
                            llvm::raw_string_ostream rso(str);
                            I.print(rso);
                            //errs()<<str<<"\n";

                            str=regex_replace(str,regex("[,]"),"");
                            vector<string> v;
                            stringstream ss(str);
                            string tmp;
                            while(ss>>tmp) v.push_back(tmp);
                            

                            cout<<"store "<<v[2]<<" "<<v[4]<<"\n";

                        }
                        else if (isa<GetElementPtrInst>(I))
                            errs()<<I<<"\n";
                    }
                }

                if(F.isDeclaration()){
                    //errs() << "Library Function Name: "<<F.getName()<<"\n";
                    if (blacklisted_functions.count(F.getName().str())){
                        errs() << F.getName() <<" is Vulnerable Function\n";
                    }
                        
                    if (F.getName().str()=="read"){
                        for(auto &use : F.uses()){
                            auto *userInst=use.getUser();
                            errs()<<*userInst<<"\n";
                            Value *buf=userInst->getOperand(1);
                            ConstantExpr *pCE = cast<ConstantExpr>(buf);
                            Value *firstop = pCE->getOperand(0);
                            auto *inst=cast<AllocaInst>(firstop);
                            uint64_t bufSize = DL->getTypeAllocSize(inst->getAllocatedType());

                            Value *sizeArg=userInst->getOperand(2);
                            // sizeArg->dump();
                            ConstantInt *CI = cast<ConstantInt>(sizeArg);
                            uint64_t readSize=CI->getSExtValue();

                            if (bufSize<readSize){
                                errs()<<"read(fd, buffer["<<bufSize<<"], "<<readSize<<")\n";
                                errs()<<"Buffer Overflow Detected\n";
                            }
                            else{
                                errs()<<"read(fd, buffer["<<bufSize<<"], "<<readSize<<")\n";
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
