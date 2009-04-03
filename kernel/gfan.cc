/*
Compute the Groebner fan of an ideal
$Author: monerjan $
$Date: 2009-04-03 13:49:16 $
$Header: /exports/cvsroot-2/cvsroot/kernel/gfan.cc,v 1.28 2009-04-03 13:49:16 monerjan Exp $
$Id: gfan.cc,v 1.28 2009-04-03 13:49:16 monerjan Exp $
*/

#include "mod2.h"

#ifdef HAVE_GFAN

#include "kstd1.h"
#include "intvec.h"
#include "polys.h"
#include "ideals.h"
#include "kmatrix.h"
#include "fast_maps.h"	//Mapping of ideals
#include "maps.h"
#include "iostream.h"	//deprecated

//Hacks for different working places
#define ITWM

#ifdef UNI
#include "/users/urmel/alggeom/monerjan/cddlib/include/setoper.h" //Support for cddlib. Dirty hack
#include "/users/urmel/alggeom/monerjan/cddlib/include/cdd.h"
#endif

#ifdef HOME
#include "/home/momo/studium/diplomarbeit/cddlib/include/setoper.h"
#include "/home/momo/studium/diplomarbeit/cddlib/include/cdd.h"
#endif

#ifdef ITWM
#include "/u/slg/monerjan/cddlib/include/setoper.h"
#include "/u/slg/monerjan/cddlib/include/cdd.h"
#endif

#ifndef gfan_DEBUG
#define gfan_DEBUG
#endif

//#include gcone.h

/**
*\brief Class facet
*	Implements the facet structure as a linked list
*
*/
class facet
{
	private:
		/** inner normal, describing the facet uniquely up to isomorphism */
		intvec *fNormal;		
	public:
		/** The default constructor. Do I need a constructor of type facet(intvec)? */
		facet()			
		{
			// Pointer to next facet.  */
			/* Defaults to NULL. This way there is no need to check explicitly */
			this->next=NULL; 
		}
		
		/** The default destructor */
		~facet(){;}
		
		/** Stores the facet normal \param intvec*/
		void setFacetNormal(intvec *iv){
			this->fNormal = ivCopy(iv);
			//return;
		}
		
		/** Hopefully returns the facet normal */
		intvec *getFacetNormal()
		{	
			//this->fNormal->show();	cout << endl;	
			return this->fNormal;
		}
		
		/** Method to print the facet normal*/
		void printNormal()
		{
			fNormal->show();
		}
		
		/** \brief The Groebner basis on the other side of a shared facet
		*
		* In order not to have to compute the flipped GB twice we store the basis we already get
		* when identifying search facets. Thus in the next step of the reverse search we can 
		* just copy the old cone and update the facet and the gcBasis
		*/
		ideal flibGB;		//The Groebner Basis on the other side, computed via gcone::flip
		
		bool isFlippable;	//flippable facet? Want to have cone->isflippable.facet[i]
		bool isIncoming;	//Is the facet incoming or outgoing?
		facet *next;		//Pointer to next facet
};

/**
*\brief Implements the cone structure
*
* A cone is represented by a linked list of facet normals
* @see facet
*/
/*class gcone
finally this should become s.th. like gconelib.{h,cc} to provide an API
*/
class gcone
{
	private:
		int numFacets;		//#of facets of the cone		

	public:
		/** \brief Default constructor. 
		*
		* Initialises this->next=NULL and this->facetPtr=NULL
		*/
		gcone()
		{
			this->next=NULL;
			this->facetPtr=NULL;
		}
		~gcone();		//destructor
		
		/** Pointer to the first facet */
		facet *facetPtr;	//Will hold the adress of the first facet; set by gcone::getConeNormals
		poly gcMarkedTerm; 	//marked terms of the cone's Groebner basis
		int numVars;		//#of variables in the ring
		
		/** Contains the Groebner basis of the cone. Is set by gcone::getGB(ideal I)*/
		ideal gcBasis;		//GB of the cone, set by gcone::getGB();
		gcone *next;		//Pointer to *previous* cone in search tree
		
		/** \brief Compute the normals of the cone
		*
		* This method computes a representation of the cone in terms of facet normals. It takes an ideal
		* as its input. Redundancies are automatically removed using cddlib's dd_MatrixCanonicalize.
		* Other methods for redundancy checkings might be implemented later. See Anders' diss p.44.
		* Note that in order to use cddlib a 0-th column has to be added to the matrix since cddlib expects 
		* each row to represent an inequality of type const+x1+...+xn <= 0. While computing the normals we come across
		* the set \f$ \partial\mathcal{G} \f$ which we might store for later use. C.f p71 of journal
		* As a result of this procedure the pointer facetPtr points to the first facet of the cone.
		*/
		void getConeNormals(ideal I)
		{
#ifdef gfan_DEBUG
			cout << "*** Computing Inequalities... ***" << endl;
#endif		
			//All variables go here - except ineq matrix and *v, see below
			int lengthGB=IDELEMS(I);	// # of polys in the groebner basis
			int pCompCount;			// # of terms in a poly
			poly aktpoly;
			int numvar = pVariables; 	// # of variables in a polynomial (or ring?)
			int leadexp[numvar];		// dirty hack of exp.vects
			int aktexp[numvar];
			int cols,rows; 			// will contain the dimensions of the ineq matrix - deprecated by
			dd_rowrange ddrows;
			dd_colrange ddcols;
			dd_rowset ddredrows;		// # of redundant rows in ddineq
			dd_rowset ddlinset;		// the opposite
			dd_rowindex ddnewpos;		// all to make dd_Canonicalize happy
			dd_NumberType ddnumb=dd_Real;	//Number type
			dd_ErrorType dderr=dd_NoError;	//
			// End of var declaration
#ifdef gfan_DEBUG
			cout << "The Groebner basis has " << lengthGB << " elements" << endl;
			cout << "The current ring has " << numvar << " variables" << endl;
#endif
			cols = numvar;
		
			//Compute the # inequalities i.e. rows of the matrix
			rows=0; //Initialization
			for (int ii=0;ii<IDELEMS(I);ii++)
			{
				aktpoly=(poly)I->m[ii];
				rows=rows+pLength(aktpoly)-1;
			}
#ifdef gfan_DEBUG
			cout << "rows=" << rows << endl;
			cout << "Will create a " << rows << " x " << cols << " matrix to store inequalities" << endl;
#endif
			dd_rowrange aktmatrixrow=0;	// needed to store the diffs of the expvects in the rows of ddineq
			dd_set_global_constants();
			ddrows=rows;
			ddcols=cols;
			dd_MatrixPtr ddineq; 		//Matrix to store the inequalities
			ddineq=dd_CreateMatrix(ddrows,ddcols+1); //The first col has to be 0 since cddlib checks for additive consts there
		
			// We loop through each g\in GB and compute the resulting inequalities
			for (int i=0; i<IDELEMS(I); i++)
			{
				aktpoly=(poly)I->m[i];		//get aktpoly as i-th component of I
				pCompCount=pLength(aktpoly);	//How many terms does aktpoly consist of?
				cout << "Poly No. " << i << " has " << pCompCount << " components" << endl;
		
				int *v=(int *)omAlloc((numvar+1)*sizeof(int));
				pGetExpV(aktpoly,v);	//find the exp.vect in v[1],...,v[n], use pNext(p)
				
				//Store leadexp for aktpoly
				for (int kk=0;kk<numvar;kk++)
				{
					leadexp[kk]=v[kk+1];
					//Since we need to know the difference of leadexp with the other expvects we do nothing here
					//but compute the diff below
				}
		
				
				while (pNext(aktpoly)!=NULL) //move to next term until NULL
				{
					aktpoly=pNext(aktpoly);
					pSetm(aktpoly);		//doesn't seem to help anything
					pGetExpV(aktpoly,v);
					for (int kk=0;kk<numvar;kk++)
					{
						aktexp[kk]=v[kk+1];
						//ineq[aktmatrixrow][kk]=leadexp[kk]-aktexp[kk];	//dito
						dd_set_si(ddineq->matrix[(dd_rowrange)aktmatrixrow][kk+1],leadexp[kk]-aktexp[kk]); //because of the 1st col being const 0
					}
					aktmatrixrow=aktmatrixrow+1;
				}//while
		
			} //for
		
			//Maybe add another row to contain the constraints of the standard simplex?

#ifdef gfan_DEBUG
			cout << "The inequality matrix is" << endl;
			dd_WriteMatrix(stdout, ddineq);
#endif

			// The inequalities are now stored in ddineq
			// Next we check for superflous rows
			ddredrows = dd_RedundantRows(ddineq, &dderr);
			if (dderr!=dd_NoError)			// did an error occur?
			{
				dd_WriteErrorMessages(stderr,dderr);	//if so tell us
			} else
			{
				cout << "Redundant rows: ";
				set_fwrite(stdout, ddredrows);		//otherwise print the redundant rows
			}//if dd_Error
		
			//Remove reduntant rows here!
			dd_MatrixCanonicalize(&ddineq, &ddlinset, &ddredrows, &ddnewpos, &dderr);
			ddrows = ddineq->rowsize;	//Size of the matrix with redundancies removed
			ddcols = ddineq->colsize;
#ifdef gfan_DEBUG
			cout << "Having removed redundancies, the normals now read:" << endl;
			dd_WriteMatrix(stdout,ddineq);
			cout << "Rows = " << ddrows << endl;
			cout << "Cols = " << ddcols << endl;
#endif
			/*Write the normals into class facet*/
#ifdef gfan_DEBUG
			cout << "Creating list of normals" << endl;
#endif
			/*The pointer *fRoot should be the return value of this function*/
			facet *fRoot = new facet();	//instantiate new facet
			this->facetPtr = fRoot;		//set variable facetPtr of class gcone to first facet
			facet *fAct; 			//instantiate pointer to active facet
			fAct = fRoot;		//This does not seem to do the trick. fRoot and fAct have to point to the same adress!
			std::cout << "fRoot = " << fRoot << ", fAct = " << fAct << endl;
			for (int kk = 0; kk<ddrows; kk++)
			{
				intvec *load = new intvec(numvar);	//intvec to store a single facet normal that will then be stored via setFacetNormal
				for (int jj = 1; jj <ddcols; jj++)
				{
					double *foo;
					foo = (double*)ddineq->matrix[kk][jj];	//get entry from actual position
#ifdef gfan_DEBUG
					std::cout << "fAct is " << *foo << " at " << fAct << std::endl;
#endif	
					(*load)[jj-1] = (int)*foo;		//store typecasted entry at pos jj-1 of load
					//check for flipability here
					if (jj<ddcols)				//Is this facet NOT the last facet? Writing while instead of if is a really bad idea :)
					{
						fAct->next = new facet();	//If so: instantiate new facet. Otherwise this->next=NULL due to the constructor						
					}
				}//for jj<ddcols
				/*Now load should be full and we can call setFacetNormal*/
				fAct->setFacetNormal(load);
				//fAct->printNormal();
				fAct=fAct->next;	//this should definitely not be called in the above while-loop :D
				delete load;
			}
			/*
			Now we should have a linked list containing the facet normals of those facets that are
			-irredundant
			-flipable
			Adressing is done via *facetPtr
			*/
			
			//Clean up but don't delete the return value! (Whatever it will turn out to be)
			dd_FreeMatrix(ddineq);
			set_free(ddredrows);
			free(ddnewpos);
			set_free(ddlinset);
			dd_free_global_constants();

		}//method getConeNormals(ideal I)	
		
		/*bool isParallel(int v[], intvec iv)
		{
}*/
		
		/** \brief Compute the Groebner Basis on the other side of a shared facet 
		*
		* Implements algorithm 4.3.2 from Anders' thesis.
		* As shown there it is not necessary to compute an interior point. The knowledge of the facet normal
		* suffices. A term \f$ x^\gamma \f$ of \f$ g \f$ is in \f$  in_\omega(g) \f$ iff \f$ \gamma - leadexp(g)\f$
		* is parallel to \f$ leadexp(g) \f$
		* Parallelity is checked using basic linear algebra. See gcone::isParallel.
		* Other possibilities includes  computing the rank of the matrix consisting of the vectors in question and
		* computing an interior point of the facet and taking all terms having the same weight with respect 
		* to this interior point.
		*\param ideal, facet
		* Input: a marked,reduced Groebner basis and a facet
		*/
		void flip(ideal gb, facet *f)		//Compute "the other side"
		{			
			
			intvec *fNormal = new intvec(this->numVars);	//facet normal, check for parallelity			
			fNormal = f->getFacetNormal();	//read this->fNormal;
#ifdef gfan_DEBUG
			cout << "fNormal=";
			fNormal->show();
			cout << endl;
#endif				
			/*1st step: Compute the initial ideal*/
			poly initialFormElement[IDELEMS(gb)];	//array of #polys in GB to store initial form
			ideal initialForm;
			poly aktpoly;
			intvec *check = new intvec(this->numVars);	//array to store the difference of LE and v
			
			for (int ii=0;ii<IDELEMS(gb);ii++)
			{
				aktpoly = (poly)gb->m[ii];								
				int *v=(int *)omAlloc((this->numVars+1)*sizeof(int));
				int *leadExpV=(int *)omAlloc((this->numVars+1)*sizeof(int));
				pGetExpV(aktpoly,leadExpV);	//find the leading exponent in leadExpV[1],...,leadExpV[n], use pNext(p)
				//pGetExpV(aktpoly,ivLeadExpV);
				initialFormElement[ii]=pHead(aktpoly);
				
				while(pNext(aktpoly)!=NULL)	/*loop trough terms and check for parallelity*/
				{
					aktpoly=pNext(aktpoly);	//next term
					pSetm(aktpoly);
					pGetExpV(aktpoly,v);		
					/* Convert (int)v into (intvec)check */			
					for (int jj=0;jj<this->numVars;jj++)
					{
						//cout << "v["<<jj+1<<"]="<<v[jj+1]<<endl;
						//cout << "leadExpV["<<jj+1<<"]="<<leadExpV[jj+1]<<endl;
						(*check)[jj]=v[jj+1]-leadExpV[jj+1];
					}
					cout << "check=";			
					check->show();
					cout << endl;
					if (isParallel(check,fNormal)) //pass *check when 
					{
						cout << "Parallel vector found, adding to initialFormElement" << endl;				
						initialFormElement[ii] = pAdd(initialFormElement[ii],(poly)pHead(aktpoly));
					}						
				}//while
				cout << "Initial Form=";				
				pWrite(initialFormElement[ii]);
				cout << "---" << endl;
				/*Now initialFormElement must be added to (ideal)initialForm */							
			}//for
		}//void flip(ideal gb, facet *f)
				
		/** \brief Compute a Groebner Basis
		*
		* Computes the Groebner basis and stores the result in gcone::gcBasis
		*\param ideal
		*\return void
		*/
		void getGB(ideal inputIdeal)
		{
			ideal gb;
			gb=kStd(inputIdeal,NULL,testHomog,NULL);
			idSkipZeroes(gb);
			this->gcBasis=gb;	//write the GB into gcBasis
		}
		
		ideal GenGrbWlk(ideal, ideal);	//Implementation of the Generic Groebner Walk. Needed for a) Computing the sink and b) finding search facets


		/** \brief Compute the dot product of two intvecs
		*
		* THIS IS WEIRD - BUT WORKS
		*/
		int dotProduct(intvec **a, intvec **b)				
		{
			intvec iva=*a;
			intvec ivb=*b;
			int res=0;
			for (int i=0;i<this->numVars;i++)
			{
				res = res+(iva[i]*ivb[i]);
			}
			return res;
		}

		/** \brief Check whether two intvecs are parallel
		*
		* \f$ \alpha\parallel\beta\Leftrightarrow\langle\alpha,\beta\rangle^2=\langle\alpha,\alpha\rangle\langle\beta,\beta\rangle \f$
		*/
		bool isParallel(intvec *a, intvec *b)
		{			
			//bool res=FALSE;
			//intvec iva(this->numVars);
			//intvec ivb(this->numVars);
			int lhs,rhs;
			//lhs=0;
			//rhs=0;
			//iva = a;
			//ivb = b;
			//lhs=dotProduct(&iva,&ivb)*dotProduct(&iva,&ivb);
			//lhs=dotProduct(&iva,&iva)*dotProduct(&ivb,&ivb);
			lhs=dotProduct(&a,&b)*dotProduct(&a,&b);
			rhs=dotProduct(&a,&a)*dotProduct(&b,&b);
			cout << "LHS="<<lhs<<", RHS="<<rhs<<endl;
			if (lhs==rhs)
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}
		}//bool isParallel
		
		/** \brief Check two intvecs for equality --- PROBABLY NOT NEEDED
		*
		* \f$ \alpha=\beta\Leftrightarrow\langle\alpha-\beta,\alpha-\beta\rangle=0 \f$
		*/
		bool isEqual(intvec a, intvec b)
		{
			intvec *ivdiff;
			int res;
			
			ivdiff=ivSub(&a,&b);
			cout << "ivdiff=";
			ivdiff->show();
			cout << endl;
			//res=dotProduct(ivdiff,ivdiff);
			if (res==0)
			{
				return TRUE;
			}
			else
			{
				return FALSE;
			}			
		}//bool isEqual
};//class gcone

ideal gfan(ideal inputIdeal)
{
	int numvar = pVariables; 
	
	#ifdef gfan_DEBUG
	cout << "Now in subroutine gfan" << endl;
	#endif
	ring inputRing=currRing;	// The ring the user entered
	ring rootRing;			// The ring associated to the target ordering
	ideal res;
	//matrix ineq; 			//Matrix containing the boundary inequalities
	facet *fRoot;
	
	/*
	1. Select target order, say dp.
	2. Compute GB of inputIdeal wrt target order -> newRing, setCurrRing etc...
	3. getConeNormals
	*/

	
	/* Construct a new ring which will serve as our root
	Does not yet work as expected. Will work fine with order dp,Dp but otherwise hangs in getGB
	*/
	rootRing=rCopy0(currRing);
	rootRing->order[0]=ringorder_dp;
	rComplete(rootRing);
	//rChangeCurrRing(rootRing);
	ideal rootIdeal;
	/* Fetch the inputIdeal into our rootRing */
	map m=(map)idInit(IDELEMS(inputIdeal),0);
	//rootIdeal=fast_map(inputIdeal,inputRing,(ideal)m, currRing);
#ifdef gfan_DEBUG
	cout << "Root ideal is " << endl;
	idPrint(rootIdeal);
	cout << "The current ring is " << endl;
	rWrite(rootRing);
	cout << endl;
#endif	
	
	gcone *gcRoot = new gcone();	//Instantiate the sink
	gcone *gcAct;
	gcAct = gcRoot;
	gcAct->numVars=pVariables;
	gcAct->getGB(inputIdeal);
	gcAct->getConeNormals(gcAct->gcBasis);	//hopefully compute the normals
	gcAct->flip(gcAct->gcBasis,gcAct->facetPtr);
	/*Now it is time to compute the search facets, respectively start the reverse search.
	But since we are in the root all facets should be search facets. IS THIS TRUE?
	MIND: AS OF NOW, THE LIST OF FACETS IS NOT PURGED OF NON-FLIPPAPLE FACETS
	*/
	
	/*As of now extra.cc expects gfan to return type ideal. Probably this will change in near future.
	The return type will then be of type LIST_CMD
	Assume gfan has finished, thus we have enumerated all the cones
	Create an array of size of #cones. Let each entry in the array contain a pointer to the respective
	Groebner Basis and merge this somehow with LIST_CMD
	=> Count the cones!
	*/
	
	res=gcAct->gcBasis;
	//cout << fRoot << endl;
	return res;
	//return GBlist;
}
/*
Since gfan.cc is #included from extra.cc there must not be a int main(){} here
*/
#endif
