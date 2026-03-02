"""
Plugin hooks for the MetaFFI Python3 plugin.

Invoked by the CLI installer:
  python plugin_hooks.py --check-prerequisites
  python plugin_hooks.py --setup-environment
  python plugin_hooks.py --pre-uninstall
"""

import os
import platform
import shutil
import sys


def check_prerequisites() -> bool:
	"""Return True if prerequisites met. Print message and return False if not."""

	if platform.system() == 'Windows':
		python3_path = shutil.which('python3')
		python_path = shutil.which('python')
		if python3_path is None and python_path is None:
			print('Python3 is not installed')
			return False

	elif platform.system() == 'Linux':
		python3_path = shutil.which('python3')
		if python3_path is None:
			print('Python3 is not installed')
			return False

	else:
		print(f'Unsupported platform: {platform.system()}')
		return False

	return True


def setup_environment():
	"""Called after files are installed. Set env vars, etc."""

	# Python3 plugin currently needs no environment setup
	pass


def pre_uninstall():
	"""Called before plugin directory is removed. Clean up env vars, etc."""

	# Python3 plugin currently has no environment cleanup needed
	pass


if __name__ == "__main__":
	if len(sys.argv) < 2:
		print("Usage: python plugin_hooks.py --check-prerequisites|--setup-environment|--pre-uninstall")
		sys.exit(1)

	action = sys.argv[1]

	if action == '--check-prerequisites':
		ok = check_prerequisites()
		sys.exit(0 if ok else 1)

	elif action == '--setup-environment':
		setup_environment()
		sys.exit(0)

	elif action == '--pre-uninstall':
		pre_uninstall()
		sys.exit(0)

	else:
		print(f"Unknown action: {action}")
		sys.exit(1)
