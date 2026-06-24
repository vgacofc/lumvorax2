# Rapport d'Analyse Bob (Mode Fallback)

Erreur: Command failed: python3 /home/lvx/LVX/lumvorax2/src/MDBAI/bob-cli/bob_analyzer.py /home/lvx/LVX/lumvorax2/src/MDBAI/forensic/bob-tasks/mdbai-9bb25280-a471-40e3-ae1a-fd7628d0018d/bob-task.json
[BOB-CLI] ⚠️  OPENAI_API_KEY manquant - mode fallback
Traceback (most recent call last):
  File "/home/lvx/LVX/lumvorax2/src/MDBAI/bob-cli/bob_analyzer.py", line 192, in <module>
    main()
  File "/home/lvx/LVX/lumvorax2/src/MDBAI/bob-cli/bob_analyzer.py", line 186, in main
    analysis = analyzer.analyze()
               ^^^^^^^^^^^^^^^^^^
  File "/home/lvx/LVX/lumvorax2/src/MDBAI/bob-cli/bob_analyzer.py", line 47, in analyze
    with open(source_file, 'r') as f:
         ^^^^^^^^^^^^^^^^^^^^^^
FileNotFoundError: [Errno 2] No such file or directory: '/home/lvx/LVX/lumvorax2/src/MDBAI/forensic/bob-tasks/mdbai-9bb25280-a471-40e3-ae1a-fd7628d0018d/source-code-complete.txt'

