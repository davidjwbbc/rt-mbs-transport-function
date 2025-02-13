# License: 5G-MAG Public License (v1.0)
# Author(s): Dev Audsin <dev.audsin@bbc.co.uk>
# Copyright: (C) 2024 British Broadcasting Corporation
#
# Licensed under the License terms and conditions for use, reproduction, and
# distribution of 5G-MAG software (the “License”).  You may not use this file
# except in compliance with the License.  You may obtain a copy of the License
# at https://www.5g-mag.com/reference-tools.  Unless required by applicable
# law or agreed to in writing, software distributed under the License is
# distributed on an “AS IS” BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.
#
# See the License for the specific language governing permissions and
# limitations under the License.


test('test_object_store', 
    executable('testObjectStore', 'test_ObjectStore.cc', test_source_object_store, install:false, include_directories:[libmbstf_libinc, libinc])
    ,verbose: true, timeout: 600, protocol: 'exitcode')


#test_object_store = executable('testObjectStore', 'test_ObjectStore.cc', test_source_object_store, install:false, include_directories:[libmbstf_libinc, libinc])
#test('run_test_object_store', executable('testObjectStore'))
