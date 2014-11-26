Ce dossier comprend les modèles utilisés par le simulateur Gazebo.

worlds/
	robomovies.world correspond à la table de jeu

models/
	Contient le fichier de modèle du robot (rosebot) et les divers objets de la table de jeu : ampoule, pieds, claps etc.

plugins/
	modules de controle de la simulation.

#####################
** Instaler Gazebo **
#####################

gazebo s'intalle sous Linux/Ubuntu (pas de version windows). 
Il est conseillé d'être sous Ubuntu trusty 14.04
Installation facile décrite ici :
http://gazebosim.org/tutorials?tut=install

Gazebo centralise les objets des la silumation dans ~/.gazebo

#####################
** Lancer Gazebo **
#####################

1/ Il est nécéssaire de créer des liens dans ~/.gazebo vers les objets du dépôt github:
exécuter : 
./setup_sim.sh

2/ Lancer gazebo sur robomovies.world depuis ~/.gazebo pour démarrer la simulation:

cd ~/.gazebo
gazebo robomovies.world



