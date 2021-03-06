.. index:: rods, creating rods, curves, furset, wmFigaro node, nodes:wmFigaro, nodes:wmFigRodNode, wmFigRodNode, volumetric collisions, collisions, 


.. _create_rods:

Create your rods
================
You have two options to create rods. You can create rods: 

* 	from a :ref:`specific set of curves<creating_rods_from_curves>`, or 
* 	from the :ref:`entire furset<creating_rods_from_a_furset>`. 	
 	This creates rods from a subset of the hairs, not all the hairs on the furset!

You can also :ref:`add rods to an existing Figaro node<adding_rods_to_existing_nodes>`. 


Creating rods
--------------

.. _creating_rods_from_curves:

To create rods from a set of curves:

#.	Select the curves to base the rods on.

	For example, you could create a set of curves from the groom using Barbershop's Wire-Pin tool, and then use these as the basis to create your rods. 
 	
 	.. note:: Select the curves themselves, rather than the group containing them. 

#. 	From the **Figaro** menu, select **Create Rods from NURBS curves**.

	Figaro creates a set of rods based on the curves.
	
.. _creating_rods_from_a_furset:	
	
To create rods from a furset:

#. 	Select the furset node (not the subd node).

#. 	From the **Figaro** menu, select **Create Rods from Barbershop Node**.


==========================================	===========================================
.. image:: images/rods_creation_before.png	.. image:: images/rods_creation_after.png
*Initial state of fur set*			*Rods created from clump curves*
==========================================	===========================================

Once you have created the rods, you will see the following new nodes:

* 	:ref:`wmFigaro<attributes_wmFigaroNode>` - this holds the overall solver information that affects all rods within the node. 
*	:ref:`wmFigRodNode<attributes_wmFigRodNode>` - this controls each subset of rods. 

You can now configure the rod attributes. In particular, you should:
 
* 	in the :ref:`wmFigaro<attributes_wmFigaroNode>`, set:
	
	* 	the **Number of Threads** to 8.
	* 	whether to use standard or volumetric collisions. To use volumetric collisions, turn **Volumetric Collisions** on, and set the volumetric sim details, especially the **Flip** and **Slip**. 
	
*	in the :ref:`wmFigRodNode<attributes_wmFigRodNode>`, set:
	
	* the **Percentage of Barbershop Strands** to create rods for.
	* the **Young's Modulus** (stiffness) and **Shear Modulus** (resistence to twisting) for the rods. 
	* damping details for the rods. 


.. _adding_rods_to_existing_nodes:

Adding rods to existing nodes
+++++++++++++++++++++++++++++

To add rods to an existing Figaro node:

#.	Select the wmFigRodNode to add the rods to.

#.	Create the rods as normal, either from a furset or from a set of curves. 

